"""Formatea el resultado final como texto legible con etiquetas Persona 1, Persona 2..."""

from typing import Optional


class FormateadorDeTranscripcion:
    """
    Convierte el resultado de WhisperX en texto legible con etiquetas amigables
    (Persona 1, Persona 2, ...).

    A diferencia de un formateo por segmento (que le asigna un solo hablante a todo
    el segmento de Whisper), este formateador trabaja a nivel de PALABRA: usa el
    hablante que la diarizacion asigno a cada palabra y corta el turno justo donde
    cambia quien habla. Esto evita que un cambio de hablante en medio de un segmento
    quede atribuido a una sola persona (problema tipico con 3+ hablantes).
    """

    def __init__(self, prefijo_hablante: str = "Persona", min_palabras_turno: int = 2):
        self.prefijo_hablante = prefijo_hablante
        # Turnos mas cortos que esto (en palabras) se consideran "ruido" de la
        # diarizacion y se absorben en el hablante vecino (suavizado anti-parpadeo).
        self.min_palabras_turno = min_palabras_turno
        self._mapa_hablantes: dict = {}

    def _nombre(self, speaker_id: Optional[str]) -> str:
        if not speaker_id:
            return f"{self.prefijo_hablante} ?"
        if speaker_id not in self._mapa_hablantes:
            self._mapa_hablantes[speaker_id] = f"{self.prefijo_hablante} {len(self._mapa_hablantes) + 1}"
        return self._mapa_hablantes[speaker_id]

    @staticmethod
    def _mmss(segundos: float) -> str:
        m = int(segundos // 60)
        s = int(segundos % 60)
        return f"{m:02d}:{s:02d}"

    # ------------------------------------------------------------------ palabras
    def _aplanar_palabras(self, resultado: dict) -> list:
        """
        Devuelve una lista plana de palabras [{word, start, end, speaker}, ...].
        Rellena el hablante faltante con el del segmento o el de la palabra previa,
        y los timestamps faltantes con el ultimo tiempo conocido.
        """
        palabras = []
        ult_speaker = None
        ult_tiempo = 0.0

        for seg in resultado.get("segments", []):
            seg_speaker = seg.get("speaker")
            lista_w = seg.get("words") or []

            if not lista_w:
                # Segmento sin palabras alineadas: lo tratamos como un bloque unico.
                texto = (seg.get("text") or "").strip()
                if texto:
                    speaker = seg_speaker or ult_speaker
                    ult_speaker = speaker or ult_speaker
                    palabras.append({
                        "word": texto,
                        "start": seg.get("start", ult_tiempo),
                        "end": seg.get("end", seg.get("start", ult_tiempo)),
                        "speaker": speaker,
                    })
                    ult_tiempo = seg.get("end", ult_tiempo)
                continue

            for w in lista_w:
                texto = (w.get("word") or "").strip()
                if not texto:
                    continue
                speaker = w.get("speaker") or seg_speaker or ult_speaker
                ult_speaker = speaker or ult_speaker
                start = w.get("start", ult_tiempo)
                end = w.get("end", start)
                ult_tiempo = end
                palabras.append({
                    "word": texto,
                    "start": start,
                    "end": end,
                    "speaker": speaker,
                })

        return palabras

    def _suavizar(self, palabras: list):
        """
        Anti-parpadeo: si una corrida de palabras del mismo hablante es muy corta
        (< min_palabras_turno) y esta rodeada por OTRO hablante igual de ambos lados,
        se reasigna a ese hablante vecino. Modifica la lista in-place.
        """
        if self.min_palabras_turno <= 1 or len(palabras) < 3:
            return

        # Agrupar indices por corridas de mismo hablante.
        corridas = []  # (speaker, [indices])
        for i, p in enumerate(palabras):
            if corridas and corridas[-1][0] == p["speaker"]:
                corridas[-1][1].append(i)
            else:
                corridas.append((p["speaker"], [i]))

        for k in range(1, len(corridas) - 1):
            speaker, indices = corridas[k]
            prev_speaker = corridas[k - 1][0]
            next_speaker = corridas[k + 1][0]
            if (len(indices) < self.min_palabras_turno
                    and prev_speaker == next_speaker
                    and prev_speaker is not None):
                for i in indices:
                    palabras[i]["speaker"] = prev_speaker

    def _construir_turnos(self, resultado: dict) -> list:
        """Agrupa palabras consecutivas del mismo hablante en turnos."""
        palabras = self._aplanar_palabras(resultado)
        if not palabras:
            return []
        self._suavizar(palabras)

        turnos = []
        for p in palabras:
            if turnos and turnos[-1]["speaker"] == p["speaker"]:
                turnos[-1]["end"] = p["end"]
                turnos[-1]["texto"].append(p["word"])
            else:
                turnos.append({
                    "speaker": p["speaker"],
                    "start": p["start"],
                    "end": p["end"],
                    "texto": [p["word"]],
                })

        for t in turnos:
            t["texto"] = " ".join(t["texto"]).strip()
        return [t for t in turnos if t["texto"]]

    # ------------------------------------------------------------------ salida
    def formatear(self, resultado: dict) -> str:
        """
        Genera texto con formato:
            [00:00 - 00:05] Persona 1: Hola, como estas?
            [00:06 - 00:10] Persona 2: Muy bien, gracias.
        """
        self._mapa_hablantes = {}
        self._turnos_cache = self._construir_turnos(resultado)

        lineas = []
        hablante_anterior = None
        for t in self._turnos_cache:
            nombre = self._nombre(t["speaker"])
            timestamp = f"[{self._mmss(t['start'])} - {self._mmss(t['end'])}]"
            if hablante_anterior and nombre != hablante_anterior:
                lineas.append("")
            lineas.append(f"{timestamp} {nombre}: {t['texto']}")
            hablante_anterior = nombre

        return "\n".join(lineas)

    def resumen_participacion(self, resultado: dict) -> dict:
        """
        Devuelve dict {nombre: segundos_hablados} (aprox.), util para ver cuanto
        participo cada persona. Usa los turnos ya calculados en formatear().
        """
        turnos = getattr(self, "_turnos_cache", None)
        if turnos is None:
            turnos = self._construir_turnos(resultado)

        conteo: dict = {}
        for t in turnos:
            nombre = self._nombre(t["speaker"])
            dur = max(0.0, t["end"] - t["start"])
            conteo[nombre] = round(conteo.get(nombre, 0.0) + dur, 1)
        return conteo

    @staticmethod
    def guardar(texto: str, ruta_salida: str):
        with open(ruta_salida, "w", encoding="utf-8") as f:
            f.write(texto)
