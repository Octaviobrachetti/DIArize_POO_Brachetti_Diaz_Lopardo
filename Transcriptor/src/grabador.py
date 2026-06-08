"""Grabador de audio desde el microfono del sistema."""

import tempfile
import wave
from pathlib import Path
from typing import Optional

import numpy as np
import sounddevice as sd


class GrabadorDeAudio:
    """
    Captura audio del microfono por defecto del sistema.

    Uso tipico:
        grabador = GrabadorDeAudio()
        grabador.iniciar()
        # ... el usuario habla ...
        ruta_wav = grabador.detener()  # devuelve Path al .wav temporal
    """

    SAMPLE_RATE = 16_000  # 16 kHz: suficiente para voz, es lo que espera AssemblyAI
    CHANNELS = 1          # mono

    def __init__(self):
        self._frames: list = []
        self._stream: Optional[sd.InputStream] = None

    @property
    def grabando(self) -> bool:
        return self._stream is not None and self._stream.active

    def iniciar(self):
        """Abre el stream de entrada y empieza a capturar muestras."""
        self._frames = []
        self._stream = sd.InputStream(
            samplerate=self.SAMPLE_RATE,
            channels=self.CHANNELS,
            dtype="int16",
            callback=self._callback,
        )
        self._stream.start()

    def _callback(self, indata: np.ndarray, frames: int, time, status) -> None:
        self._frames.append(indata.copy())

    def detener(self) -> Optional[str]:
        """
        Detiene la grabacion, guarda un .wav en disco temporal y retorna su ruta.
        Retorna None si no se capturo nada.
        """
        if self._stream:
            self._stream.stop()
            self._stream.close()
            self._stream = None

        if not self._frames:
            return None

        audio = np.concatenate(self._frames, axis=0)
        ruta = Path(tempfile.mktemp(prefix="grabacion_", suffix=".wav"))

        with wave.open(str(ruta), "wb") as wf:
            wf.setnchannels(self.CHANNELS)
            wf.setsampwidth(2)          # int16 = 2 bytes por muestra
            wf.setframerate(self.SAMPLE_RATE)
            wf.writeframes(audio.tobytes())

        return str(ruta)
