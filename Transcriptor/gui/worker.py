"""QThread worker para no bloquear la UI durante operaciones pesadas."""

from typing import Callable

from PySide6.QtCore import QThread, Signal


class TareaWorker(QThread):
    """
    Ejecuta una funcion en un hilo aparte. Emite:
        progreso(etapa: str, pct: int) -- progreso intermedio
        terminado(resultado) -- al finalizar OK (cualquier objeto)
        error(mensaje: str) -- si algo falla
    """

    progreso = Signal(str, int)
    terminado = Signal(object)
    error = Signal(str)

    def __init__(self, funcion: Callable, *args, **kwargs):
        super().__init__()
        self._funcion = funcion
        self._args = args
        self._kwargs = kwargs

    def run(self):
        try:
            resultado = self._funcion(*self._args, **self._kwargs)
            self.terminado.emit(resultado)
        except Exception as e:
            import traceback
            self.error.emit(f"{type(e).__name__}: {e}\n\n{traceback.format_exc()}")
