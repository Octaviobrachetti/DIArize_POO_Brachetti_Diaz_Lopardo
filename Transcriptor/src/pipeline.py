"""Orquesta el proceso completo: transcripcion + alineacion + diarizacion + formateo."""

from pathlib import Path
from typing import Optional

import whisperx

from src.transcriptor import AudioTranscriber
from src.alineador import AlineadorDeHablantes
from src.diarizador import DiarizadorDeHablantes
from src.formateador import FormateadorDeTranscripcion


class PipelineDeTranscripcion:
    """
    Orquesta:
        1. Transcripcion del audio (Whisper)
        2. Alineacion de timestamps por palabra (wav2vec2)
        3. Diarizacion de hablantes (PyAnnote) -- opcional si no hay HF token
        4. Asignacion de hablantes a cada segmento
        5. Formateo a texto legible

    Si hf_token es vacio o None, se salta la diarizacion (sale sin Persona 1/2/...).
    """

    def __init__(
        self,
        modelo_whisper: str = "medium",
        idioma: str = "es",
        hf_token: str = "",
        dispositivo: str = "cpu",
        num_hablantes: Optional[int] = None,
        min_hablantes: Optional[int] = None,
        max_hablantes: Optional[int] = None,
        prefijo_hablante: str = "Persona",
        compute_type: Optional[str] = None,
        progreso: Optional[callable] = None,
    ):
        self.transcriptor = AudioTranscriber(
            modelo_whisper=modelo_whisper,
            dispositivo=dispositivo,
            idioma=idioma,
            compute_type=compute_type,
        )
        self.alineador = AlineadorDeHablantes(dispositivo=dispositivo)
        self.diarizador = None
        if hf_token:
            self.diarizador = DiarizadorDeHablantes(
                hf_token=hf_token,
                dispositivo=dispositivo,
                num_hablantes=num_hablantes,
                min_hablantes=min_hablantes,
                max_hablantes=max_hablantes,
            )
        self.formateador = FormateadorDeTranscripcion(prefijo_hablante=prefijo_hablante)
        self.progreso = progreso or (lambda etapa, pct: None)

    def ejecutar(self, ruta_audio: str) -> dict:
        """
        Ejecuta todo el pipeline.

        Returns:
            dict con keys: 'texto', 'resultado', 'participacion'
                texto: string formateado con [tiempos] Persona N: ...
                resultado: dict crudo de whisperx (segments con start/end/speaker)
                participacion: dict {nombre: cantidad_segmentos}
        """
        ruta_audio = str(Path(ruta_audio))
        self.progreso("Cargando modelo Whisper", 5)

        # 1+2: transcripcion
        resultado, audio = self.transcriptor.transcribir(ruta_audio)
        self.progreso("Transcripcion lista", 40)

        # 3: alineacion
        self.progreso("Alineando timestamps", 45)
        resultado_alineado = self.alineador.alinear(resultado, audio)
        self.progreso("Alineacion lista", 65)

        # 4: diarizacion (opcional)
        if self.diarizador is not None:
            self.progreso("Identificando hablantes", 70)
            diarize_df = self.diarizador.diarizar(audio)
            # fill_nearest=True: a los segmentos que caen en un hueco de la
            # diarizacion (comun con 3+ personas y turnos rapidos) les asigna
            # el hablante mas cercano en vez de dejarlos sin etiqueta ("Persona ?").
            resultado_final = whisperx.assign_word_speakers(
                diarize_df, resultado_alineado, fill_nearest=True
            )
            self.progreso("Diarizacion lista", 90)
        else:
            resultado_final = resultado_alineado

        # 5: formateo
        texto = self.formateador.formatear(resultado_final)
        participacion = self.formateador.resumen_participacion(resultado_final)
        self.progreso("Listo", 100)

        return {
            "texto": texto,
            "resultado": resultado_final,
            "participacion": participacion,
        }
