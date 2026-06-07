"""Carga el modelo Whisper y transcribe archivos de audio."""

from pathlib import Path

import whisperx


class AudioTranscriber:
    """
    Carga el modelo de transcripcion WhisperX y transcribe archivos de audio.

    Atributos:
        modelo_whisper (str): Tamano del modelo (tiny, base, small, medium, large-v2, large-v3)
        dispositivo (str): 'cuda' para GPU, 'cpu' para procesador
        idioma (str): Codigo de idioma ('es', 'en', etc.) o None para deteccion automatica
        batch_size (int): Cantidad de segmentos procesados en paralelo
        compute_type (str): float16 (GPU), int8 (CPU/poca VRAM), float32 (default CPU)
    """

    MODELOS_DISPONIBLES = ["tiny", "base", "small", "medium", "large-v2", "large-v3"]

    def __init__(
        self,
        modelo_whisper: str = "medium",
        dispositivo: str = "cpu",
        idioma: str = "es",
        batch_size: int = 8,
        compute_type: str = None,
    ):
        if modelo_whisper not in self.MODELOS_DISPONIBLES:
            raise ValueError(
                f"Modelo '{modelo_whisper}' no valido. "
                f"Opciones: {self.MODELOS_DISPONIBLES}"
            )

        self.modelo_whisper = modelo_whisper
        self.dispositivo = dispositivo
        self.idioma = idioma
        self.batch_size = batch_size
        self.compute_type = compute_type or ("float16" if dispositivo == "cuda" else "int8")
        self._pipeline = None

    def cargar_modelo(self):
        """Carga el modelo de Whisper en memoria (puede demorar la primera vez)."""
        print(f"[Whisper] Cargando '{self.modelo_whisper}' en {self.dispositivo} ({self.compute_type})...")
        self._pipeline = whisperx.load_model(
            self.modelo_whisper,
            self.dispositivo,
            compute_type=self.compute_type,
            language=self.idioma,
        )

    def transcribir(self, ruta_audio: str):
        """
        Transcribe el archivo de audio.

        Returns:
            Tupla (resultado_dict, audio_array)
        """
        if self._pipeline is None:
            self.cargar_modelo()

        if not Path(ruta_audio).exists():
            raise FileNotFoundError(f"No se encontro el archivo: {ruta_audio}")

        print(f"[Whisper] Transcribiendo '{Path(ruta_audio).name}'...")
        audio = whisperx.load_audio(ruta_audio)
        resultado = self._pipeline.transcribe(
            audio,
            batch_size=self.batch_size,
            language=self.idioma,
        )
        idioma_detectado = resultado.get("language", self.idioma or "desconocido")
        print(f"[Whisper] Idioma: {idioma_detectado} | Segmentos: {len(resultado['segments'])}")
        return resultado, audio
