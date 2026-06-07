"""Paquete del transcriptor de audio con diarizacion e IA."""

import os
import shutil
from pathlib import Path


def _ensure_ffmpeg_in_path():
    """
    Garantiza que el ejecutable ffmpeg sea accesible desde subprocess.
    whisperx.load_audio() llama a ffmpeg via subprocess.Popen y depende del PATH.
    """
    if shutil.which("ffmpeg"):
        return

    # 1) Instalacion via WinGet (caso default cuando se hizo: winget install Gyan.FFmpeg)
    base = Path(os.environ.get("LOCALAPPDATA", "")) / "Microsoft" / "WinGet" / "Packages"
    if base.exists():
        for exe in base.glob("Gyan.FFmpeg_*/ffmpeg-*-full_build/bin/ffmpeg.exe"):
            os.environ["PATH"] = str(exe.parent) + os.pathsep + os.environ.get("PATH", "")
            return

    # 2) Fallbacks comunes (instalaciones manuales)
    for candidato in [r"C:\ffmpeg\bin", r"C:\Program Files\ffmpeg\bin"]:
        if Path(candidato, "ffmpeg.exe").exists():
            os.environ["PATH"] = candidato + os.pathsep + os.environ.get("PATH", "")
            return


_ensure_ffmpeg_in_path()

# NOTA: NO importamos las clases aca a proposito. Antes este __init__ hacia
# `from src.pipeline import ...` lo que arrastraba whisperx + torch + pyannote
# al solo tocar el paquete `src`, demorando ~5 min el arranque de la GUI.
# Cada consumidor importa lo que necesita: `from src.transcriptor import ...`.
