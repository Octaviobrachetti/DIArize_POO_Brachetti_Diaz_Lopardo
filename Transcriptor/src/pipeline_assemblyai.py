"""Pipeline de transcripcion + diarizacion usando AssemblyAI (API en la nube)."""

from pathlib import Path
from typing import Callable, Optional

from src.formateador import FormateadorDeTranscripcion


class PipelineAssemblyAI:
    """
    Pipeline de transcripcion y diarizacion usando la API de AssemblyAI.

    Misma interfaz publica que PipelineDeTranscripcion:
        ejecutar(ruta_audio) -> {"texto": str, "resultado": dict, "participacion": dict}

    Ventajas sobre el pipeline local (WhisperX + PyAnnote):
        - No requiere GPU ni descarga de modelos de varios GB
        - Resultado en segundos en vez de minutos
        - Mejor calidad de diarizacion
        - Sin dependencias pesadas (torch, whisperx)
    """

    IDIOMAS = {"es", "en", "pt", "fr", "it", "de"}

    def __init__(
        self,
        api_key: str,
        idioma: str = "es",
        num_hablantes: Optional[int] = None,
        prefijo_hablante: str = "Persona",
        progreso: Optional[Callable] = None,
    ):
        if not api_key:
            raise ValueError(
                "Se necesita una API key de AssemblyAI.\n"
                "Obtene una gratis en https://www.assemblyai.com"
            )
        self.api_key = api_key
        self.idioma = idioma if idioma in self.IDIOMAS else "es"
        self.num_hablantes = num_hablantes
        self.formateador = FormateadorDeTranscripcion(prefijo_hablante=prefijo_hablante)
        self.progreso = progreso or (lambda etapa, pct: None)

    def ejecutar(self, ruta_audio: str) -> dict:
        """
        Sube el audio a AssemblyAI, espera el resultado y lo devuelve
        en el mismo formato que PipelineDeTranscripcion.ejecutar().
        """
        import assemblyai as aai

        aai.settings.api_key = self.api_key

        config = aai.TranscriptionConfig(
            speaker_labels=True,
            language_code=self.idioma,
            speakers_expected=self.num_hablantes,
        )

        self.progreso("Subiendo y procesando audio...", 10)
        transcriber = aai.Transcriber()

        # transcribe() es sincrono: sube el archivo, espera el resultado y lo devuelve.
        # La barra de progreso corre en modo indeterminado (pulsante) en la GUI
        # mientras dura esta llamada (segundos, no minutos como el pipeline local).
        transcript = transcriber.transcribe(str(Path(ruta_audio)), config=config)

        if transcript.status == aai.TranscriptStatus.error:
            raise RuntimeError(f"Error de AssemblyAI: {transcript.error}")

        self.progreso("Formateando resultado...", 88)
        resultado = self._convertir_formato(transcript)
        texto = self.formateador.formatear(resultado)
        participacion = self.formateador.resumen_participacion(resultado)
        self.progreso("Listo", 100)

        return {"texto": texto, "resultado": resultado, "participacion": participacion}

    def _convertir_formato(self, transcript) -> dict:
        """Convierte el resultado de AssemblyAI al formato interno del formateador."""
        if not transcript.utterances:
            # Sin diarizacion: construir un solo segmento con todas las palabras
            words = [
                {
                    "word": w.text,
                    "start": w.start / 1000.0,
                    "end": w.end / 1000.0,
                    "speaker": None,
                }
                for w in (transcript.words or [])
                if w.text and w.text.strip()
            ]
            if not words:
                return {"segments": []}
            return {
                "segments": [{
                    "speaker": None,
                    "start": words[0]["start"],
                    "end": words[-1]["end"],
                    "text": transcript.text or "",
                    "words": words,
                }]
            }

        return {
            "segments": [
                {
                    "speaker": u.speaker,
                    "start": u.start / 1000.0,
                    "end": u.end / 1000.0,
                    "text": u.text,
                    "words": [
                        {
                            "word": w.text,
                            "start": w.start / 1000.0,
                            "end": w.end / 1000.0,
                            "speaker": u.speaker,
                        }
                        for w in (u.words or [])
                        if w.text and w.text.strip()
                    ],
                }
                for u in transcript.utterances
            ]
        }
