"""Identifica que persona habla en cada momento (PyAnnote)."""

import torch
from whisperx.diarize import DiarizationPipeline


class DiarizadorDeHablantes:
    """
    Identifica y separa los diferentes hablantes del audio usando PyAnnote.
    Requiere un token de HuggingFace y haber aceptado los terminos del modelo
    pyannote/speaker-diarization-community-1.
    """

    def __init__(
        self,
        hf_token: str,
        dispositivo: str = "cpu",
        num_hablantes: int = None,
        min_hablantes: int = None,
        max_hablantes: int = None,
    ):
        if not hf_token:
            raise ValueError(
                "Se necesita un token de HuggingFace para la diarizacion.\n"
                "Creá uno en: https://huggingface.co/settings/tokens\n"
                "Acepta los terminos en: https://huggingface.co/pyannote/speaker-diarization-community-1"
            )

        self.hf_token = hf_token
        self.dispositivo = dispositivo
        self.num_hablantes = num_hablantes
        self.min_hablantes = min_hablantes
        self.max_hablantes = max_hablantes
        self._pipeline = None

    def cargar_modelo(self):
        print("[Diarizador] Cargando modelo PyAnnote...")
        self._pipeline = DiarizationPipeline(
            token=self.hf_token,
            device=torch.device(self.dispositivo),
        )

    def diarizar(self, audio_o_ruta):
        """
        Identifica los segmentos de cada hablante.

        Args:
            audio_o_ruta: Ruta al audio o array numpy (mas eficiente reusar el array)
        """
        if self._pipeline is None:
            self.cargar_modelo()

        diarize_df = self._pipeline(
            audio_o_ruta,
            num_speakers=self.num_hablantes,
            min_speakers=self.min_hablantes,
            max_speakers=self.max_hablantes,
        )

        n = diarize_df["speaker"].nunique()
        print(f"[Diarizador] Hablantes detectados: {n}")
        return diarize_df
