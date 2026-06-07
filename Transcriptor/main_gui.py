"""Lanzador de la interfaz grafica del Transcriptor."""

import sys

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
