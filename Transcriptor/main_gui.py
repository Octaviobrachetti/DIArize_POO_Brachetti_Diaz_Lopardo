"""Lanzador de la interfaz grafica del Transcriptor."""

import io
import os
import sys

# pythonw.exe no tiene consola: sys.stdout/stderr son None.
# torch, tqdm y huggingface_hub intentan escribir en stdout y revientan.
# Solucion: si son None los redirigimos a devnull; si existen, forzamos UTF-8.
for _s in ("stdout", "stderr"):
    _stream = getattr(sys, _s, None)
    if _stream is None:
        setattr(sys, _s, open(os.devnull, "w", encoding="utf-8"))
    elif hasattr(_stream, "buffer"):
        setattr(sys, _s, io.TextIOWrapper(_stream.buffer, encoding="utf-8", errors="replace"))

from dotenv import load_dotenv
from PySide6.QtWidgets import QApplication

from gui.ventana_principal import VentanaPrincipal

load_dotenv()


def main():
    app = QApplication(sys.argv)
    app.setApplicationName("Transcriptor de Audio - POO")
    ventana = VentanaPrincipal()
    ventana.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
