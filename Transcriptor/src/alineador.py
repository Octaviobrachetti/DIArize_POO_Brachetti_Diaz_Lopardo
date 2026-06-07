"""Alinea timestamps por palabra con wav2vec2."""

import whisperx


class AlineadorDeHablantes:
    """
    Alinea la transcripcion con el audio para obtener timestamps precisos
    a nivel de palabra. Esto mejora mucho la asignacion de hablantes.
    """

    def __init__(self, dispositivo: str = "cpu"):
        self.dispositivo = dispositivo
        self._modelo = None
        self._metadata = None
        self._idioma_cargado = None

    def cargar_modelo(self, idioma: str):
        """Carga el modelo de alineacion para el idioma dado."""
        if self._idioma_cargado == idioma and self._modelo is not None:
            return
        print(f"[Alineador] Cargando modelo para idioma '{idioma}'...")
        self._modelo, self._metadata = whisperx.load_align_model(
            language_code=idioma,
            device=self.dispositivo,
        )
        self._idioma_cargado = idioma

    def alinear(self, resultado: dict, audio) -> dict:
        """Alinea la transcripcion para obtener timestamps por palabra."""
        idioma = resultado.get("language", "es")
        self.cargar_modelo(idioma)

        return whisperx.align(
            resultado["segments"],
            self._modelo,
            self._metadata,
            audio,
            self.dispositivo,
            return_char_alignments=False,
        )
