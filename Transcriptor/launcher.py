"""Launcher minimo del Transcriptor: lanza la GUI usando el pythonw del venv.

Este archivo se compila a Transcriptor.exe con PyInstaller. El .exe queda en la
raiz del proyecto (junto al venv) y al hacer doble click levanta la ventana
sin consola negra. No empaqueta torch ni whisperx: usa los del venv.
"""

import ctypes
import subprocess
import sys
from pathlib import Path


def _error(mensaje: str) -> None:
    ctypes.windll.user32.MessageBoxW(0, mensaje, "Transcriptor - Error", 0x10)


def main() -> int:
    # Cuando corre como .exe (PyInstaller), sys.executable apunta al .exe
    # Cuando corre como .py, sys.executable es el Python que lo ejecuta
    if getattr(sys, "frozen", False):
        base = Path(sys.executable).parent.resolve()
    else:
        base = Path(__file__).parent.resolve()

    # Buscar venv primero al lado del .exe, luego un nivel arriba (estructura del repo)
    python_exe = base / "venv" / "Scripts" / "pythonw.exe"
    if not python_exe.exists():
        python_exe = base.parent / "venv" / "Scripts" / "pythonw.exe"

    script = base / "main_gui.py"

    if not python_exe.exists():
        _error(
            f"No se encontro el entorno virtual.\n\n"
            "Buscado en:\n"
            f"  {base / 'venv'}\n"
            f"  {base.parent / 'venv'}\n\n"
            "Asegurate de tener la carpeta 'venv' al lado del Transcriptor.exe "
            "o en la carpeta que lo contiene."
        )
        return 1

    if not script.exists():
        _error(f"No se encontro main_gui.py en:\n{base}")
        return 1

    subprocess.Popen(
        [str(python_exe), str(script)],
        cwd=str(base),
        creationflags=0x08000000,  # CREATE_NO_WINDOW
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
