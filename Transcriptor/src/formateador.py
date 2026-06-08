"""Formatea el resultado final como texto legible con etiquetas Persona 1, Persona 2..."""

from typing import Optional


class FormateadorDeTranscripcion:
    """
    Convierte el resultado de WhisperX (con speakers SPEAKER_00, SPEAKER_01, ...)
    en texto legible con etiquetas amigables (Persona 1, Persona 2, ...).

    Trabaja a nivel de SEGMENTO (un hablante por segmento de Whisper). Es el metodo
    mas estable: usa el texto tal cual lo produjo Whisper y evita el parpadeo de
    hablantes que aparece al asignar palabra por palabra.
    """

    def __init__(self, prefijo_hablante: str = "Persona"):
        self.prefijo_hablante = prefijo_hablante
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

    def formatear(self, resultado: dict) -> str:
        """
        Genera texto con formato:
            [00:00 - 00:05] Persona 1: Hola, como estas?
            [00:06 - 00:10] Persona 2: Muy bien, gracias.
        """
        self._mapa_hablantes = {}
        lineas = []
        hablante_anterior = None

        for seg in resultado.get("segments", []):
            texto = seg.get("text", "").strip()
            if not texto:
                continue

            nombre = self._nombre(seg.get("speaker"))
            inicio = seg.get("start", 0)
            fin = seg.get("end", 0)
            timestamp = f"[{self._mmss(inicio)} - {self._mmss(fin)}]"

            if hablante_anterior and nombre != hablante_anterior:
                lineas.append("")

            lineas.append(f"{timestamp} {nombre}: {texto}")
            hablante_anterior = nombre

        return "\n".join(lineas)

    def resumen_participacion(self, resultado: dict) -> dict:
        """Devuelve dict {nombre: cantidad_segmentos}."""
        conteo: dict = {}
        for seg in resultado.get("segments", []):
            nombre = self._mapa_hablantes.get(seg.get("speaker"), self._nombre(seg.get("speaker")))
            conteo[nombre] = conteo.get(nombre, 0) + 1
        return conteo

    @staticmethod
    def guardar(texto: str, ruta_salida: str):
        with open(ruta_salida, "w", encoding="utf-8") as f:
            f.write(texto)
