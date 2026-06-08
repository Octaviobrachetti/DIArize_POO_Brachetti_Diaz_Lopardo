"""Orquesta el proceso completo: transcripcion + alineacion + diarizacion + formateo."""

import gc
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

    def _liberar_memoria(self):
        """Fuerza la liberacion de memoria RAM/VRAM entre etapas."""
        gc.collect()
        try:
            import torch
            if torch.cuda.is_available():
                torch.cuda.empty_cache()
                torch.cuda.ipc_collect()
        except Exception:
            pass

    def _con_fallback_cpu(self, componente, funcion, nombre_etapa, pct=50):
        """
        Ejecuta una etapa (alineacion/diarizacion) en su dispositivo actual.
        Si la GPU falla por un error de cuDNN/CUDA (ej: 'unable to find an engine'
        o 'out of memory'), libera memoria, cambia ese componente a CPU y reintenta.
        En CPU es mas lento pero no falla.
        """
        try:
            return funcion()
        except RuntimeError as e:
            if componente.dispositivo == "cpu":
                raise  # ya estabamos en CPU, no hay a donde caer: propagamos
            print(f"[Pipeline] La {nombre_etapa} fallo en GPU ({e}). Reintentando en CPU...")
            self.progreso(f"GPU sin memoria: {nombre_etapa} en CPU (mas lento)", pct)
            componente.liberar()
            self._liberar_memoria()
            componente.dispositivo = "cpu"
            return funcion()

    def liberar_todo(self):
        """
        Vacia por completo la memoria de TODOS los modelos (Whisper, alineador y
        diarizador). Se llama al terminar cada audio para que la VRAM quede limpia
        y no se acumule entre corridas.
        """
        self.transcriptor.liberar()
        self.alineador.liberar()
        if self.diarizador is not None:
            self.diarizador.liberar()
        self._liberar_memoria()

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

        try:
            # 1+2: transcripcion
            resultado, audio = self.transcriptor.transcribir(ruta_audio)
            # Liberamos el modelo de Whisper antes de seguir: en GPUs de 4GB no entra
            # Whisper + alineacion + diarizacion cargados a la vez (causa el error de
            # memoria con large-v3). Cada etapa libera su modelo al terminar.
            self.transcriptor.liberar()
            self._liberar_memoria()
            self.progreso("Transcripcion lista", 40)

            # 3: alineacion (con fallback a CPU si la GPU falla por cuDNN/memoria)
            self.progreso("Alineando timestamps", 45)
            resultado_alineado = self._con_fallback_cpu(
                self.alineador, lambda: self.alineador.alinear(resultado, audio),
                "alineacion", pct=45
            )
            self.alineador.liberar()
            self._liberar_memoria()
            self.progreso("Alineacion lista", 65)

            # 4: diarizacion (opcional, con fallback a CPU)
            if self.diarizador is not None:
                self.progreso("Identificando hablantes", 70)
                diarize_df = self._con_fallback_cpu(
                    self.diarizador, lambda: self.diarizador.diarizar(audio),
                    "diarizacion", pct=70
                )
                self.diarizador.liberar()
                self._liberar_memoria()
                resultado_final = whisperx.assign_word_speakers(diarize_df, resultado_alineado)
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
        finally:
            # Pase lo que pase (exito o error) dejamos la memoria limpia para el
            # proximo audio: vacia VRAM/RAM de todos los modelos.
            self.liberar_todo()
