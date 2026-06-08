"""Ventana principal del transcriptor."""

from __future__ import annotations

import os
import tempfile
from pathlib import Path
from typing import Optional

from PySide6.QtCore import Qt, QUrl
from PySide6.QtGui import QFont
from PySide6.QtMultimedia import (
    QAudioInput,
    QAudioOutput,
    QMediaCaptureSession,
    QMediaDevices,
    QMediaPlayer,
    QMediaRecorder,
)
from PySide6.QtWidgets import (
    QApplication,
    QComboBox,
    QFileDialog,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QMainWindow,
    QMessageBox,
    QProgressBar,
    QPushButton,
    QSpinBox,
    QSplitter,
    QStatusBar,
    QTabWidget,
    QTextEdit,
    QVBoxLayout,
    QWidget,
)

from src.asistente_ia import AsistenteIA
from gui.worker import TareaWorker

# (etiqueta visible, valor real). Hardcodeado aca para evitar importar src.transcriptor
# (que arrastra whisperx/torch al arranque). Los valores deben coincidir con
# AudioTranscriber.MODELOS_DISPONIBLES. La etiqueta avisa velocidad/uso de memoria.
_MODELOS_DISPONIBLES = [
    ("tiny (muy rapido, poco preciso)", "tiny"),
    ("base (rapido)", "base"),
    ("small (equilibrado)", "small"),
    ("medium (recomendado)", "medium"),
    ("large-v2 (lento, mucha memoria)", "large-v2"),
    ("large-v3 (lento, mucha memoria)", "large-v3"),
]


class VentanaPrincipal(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Transcriptor de Audio con IA - Proyecto POO")
        self.resize(1100, 750)

        self.ruta_audio: Optional[str] = None
        self.texto_original: str = ""
        self.pipeline = None  # PipelineDeTranscripcion, se crea bajo demanda
        self.ia: Optional[AsistenteIA] = None
        self.worker: Optional[TareaWorker] = None
        self.historial_chat: list = []  # lista de (pregunta, respuesta)

        # Grabacion de audio (QtMultimedia); se inicializa la primera vez que se usa
        self._captura = None
        self._audio_input = None
        self._grabador = None

        # Reproduccion de audio (QtMultimedia); se inicializa al primer uso
        self._player = None
        self._audio_output = None

        self._construir_ui()
        self._aplicar_estilo()
        self._cargar_ia_si_corresponde()

    # ------------------------------------------------------------------ UI
    def _construir_ui(self):
        central = QWidget()
        self.setCentralWidget(central)
        layout = QVBoxLayout(central)
        layout.setContentsMargins(12, 12, 12, 12)
        layout.setSpacing(10)

        # --- Fila de configuracion ---
        config_box = QHBoxLayout()

        self.btn_cargar = QPushButton("Cargar audio")
        self.btn_cargar.clicked.connect(self._cargar_audio)
        config_box.addWidget(self.btn_cargar)

        self.btn_grabar = QPushButton("● Grabar")
        self.btn_grabar.setToolTip("Grabar audio desde el microfono y usarlo para transcribir")
        self.btn_grabar.clicked.connect(self._toggle_grabacion)
        config_box.addWidget(self.btn_grabar)

        self.btn_reproducir = QPushButton("▶ Reproducir")
        self.btn_reproducir.setToolTip("Escuchar el audio cargado o grabado")
        self.btn_reproducir.clicked.connect(self._toggle_reproduccion)
        self.btn_reproducir.setEnabled(False)
        config_box.addWidget(self.btn_reproducir)

        self.lbl_archivo = QLabel("Ningun archivo seleccionado")
        self.lbl_archivo.setStyleSheet("color: #888;")
        config_box.addWidget(self.lbl_archivo, stretch=1)

        config_box.addWidget(QLabel("Modelo:"))
        self.combo_modelo = QComboBox()
        for etiqueta, valor in _MODELOS_DISPONIBLES:
            self.combo_modelo.addItem(etiqueta, valor)
        self.combo_modelo.setCurrentIndex(3)  # medium (recomendado)
        config_box.addWidget(self.combo_modelo)

        config_box.addWidget(QLabel("Idioma:"))
        self.combo_idioma = QComboBox()
        self.combo_idioma.addItems(["es", "en", "pt", "fr", "it", "de"])
        config_box.addWidget(self.combo_idioma)

        # --- Cantidad de personas (la fija el usuario; NO hay modo automatico
        #     porque la deteccion automatica funciona mal) ---
        config_box.addWidget(QLabel("Cantidad de personas:"))
        self.spin_personas = QSpinBox()
        self.spin_personas.setRange(1, 10)
        self.spin_personas.setValue(2)
        self.spin_personas.setToolTip(
            "Indicá cuantas personas hablan en el audio.\n"
            "Decirle el numero exacto da la mejor diarizacion."
        )
        config_box.addWidget(self.spin_personas)

        layout.addLayout(config_box)

        # --- Boton transcribir + progreso ---
        accion_box = QHBoxLayout()
        self.btn_transcribir = QPushButton("Transcribir audio")
        self.btn_transcribir.setMinimumHeight(36)
        self.btn_transcribir.clicked.connect(self._transcribir)
        self.btn_transcribir.setEnabled(False)
        accion_box.addWidget(self.btn_transcribir)

        self.progress = QProgressBar()
        self.progress.setRange(0, 100)
        self.progress.setValue(0)
        accion_box.addWidget(self.progress, stretch=1)
        layout.addLayout(accion_box)

        # --- Tabs ---
        self.tabs = QTabWidget()
        layout.addWidget(self.tabs, stretch=1)

        # Tab 1: original
        self.txt_original = self._nueva_area_texto("Aca aparece la transcripcion original con timestamps y personas...")
        self.tabs.addTab(self.txt_original, "Original")

        # Tab 2: limpio
        self.txt_limpio, self.btn_limpiar = self._tab_con_boton("Limpiar con IA")
        self.btn_limpiar.clicked.connect(self._limpiar_texto)
        self.tabs.addTab(self._wrap_tab(self.btn_limpiar, self.txt_limpio), "Limpio")

        # Tab 3: resumen
        self.txt_resumen, self.btn_resumir = self._tab_con_boton("Resumir con IA")
        self.btn_resumir.clicked.connect(self._resumir)
        self.tabs.addTab(self._wrap_tab(self.btn_resumir, self.txt_resumen), "Resumen")

        # Tab 4: analisis
        self.txt_analisis, self.btn_analizar = self._tab_con_boton("Analizar con IA")
        self.btn_analizar.clicked.connect(self._analizar)
        self.tabs.addTab(self._wrap_tab(self.btn_analizar, self.txt_analisis), "Analisis")

        # Tab 5: Q&A
        chat_widget = QWidget()
        chat_layout = QVBoxLayout(chat_widget)
        chat_layout.setContentsMargins(0, 0, 0, 0)
        self.txt_chat = QTextEdit()
        self.txt_chat.setReadOnly(True)
        self.txt_chat.setPlaceholderText("Hacele preguntas sobre lo que se dijo en el audio.")
        chat_layout.addWidget(self.txt_chat, stretch=1)
        chat_input_box = QHBoxLayout()
        self.input_pregunta = QLineEdit()
        self.input_pregunta.setPlaceholderText("Escribi tu pregunta y enter...")
        self.input_pregunta.returnPressed.connect(self._preguntar)
        chat_input_box.addWidget(self.input_pregunta, stretch=1)
        self.btn_preguntar = QPushButton("Preguntar")
        self.btn_preguntar.clicked.connect(self._preguntar)
        chat_input_box.addWidget(self.btn_preguntar)
        chat_layout.addLayout(chat_input_box)
        self.tabs.addTab(chat_widget, "Chat (Q&A)")

        # --- Fila inferior: guardar / limpiar / participacion ---
        bottom_box = QHBoxLayout()
        self.lbl_participacion = QLabel("")
        self.lbl_participacion.setStyleSheet("color: #555;")
        bottom_box.addWidget(self.lbl_participacion, stretch=1)
        self.btn_guardar = QPushButton("Guardar todo (.txt)")
        self.btn_guardar.clicked.connect(self._guardar_todo)
        self.btn_guardar.setEnabled(False)
        bottom_box.addWidget(self.btn_guardar)
        layout.addLayout(bottom_box)

        # --- Status bar ---
        self.setStatusBar(QStatusBar())
        self._set_estado("Listo. Cargá un archivo de audio para empezar.")

    def _nueva_area_texto(self, placeholder: str) -> QTextEdit:
        t = QTextEdit()
        t.setReadOnly(False)
        t.setPlaceholderText(placeholder)
        f = QFont("Consolas", 10)
        t.setFont(f)
        return t

    def _tab_con_boton(self, etiqueta_boton: str):
        boton = QPushButton(etiqueta_boton)
        boton.setEnabled(False)
        area = self._nueva_area_texto(f"Resultado de '{etiqueta_boton}'...")
        return area, boton

    def _wrap_tab(self, boton: QPushButton, area: QTextEdit) -> QWidget:
        w = QWidget()
        l = QVBoxLayout(w)
        l.setContentsMargins(0, 0, 0, 0)
        l.addWidget(boton)
        l.addWidget(area, stretch=1)
        return w

    # ------------------------------------------------------------------ grabacion
    def _init_grabador(self) -> bool:
        """Inicializa el grabador la primera vez. Devuelve False si no hay microfono."""
        if self._grabador is not None:
            return True
        if not QMediaDevices.audioInputs():
            QMessageBox.warning(self, "Sin microfono",
                                "No se detecto ningun microfono en el sistema.")
            return False
        self._captura = QMediaCaptureSession()
        self._audio_input = QAudioInput()
        self._captura.setAudioInput(self._audio_input)
        self._grabador = QMediaRecorder()
        self._captura.setRecorder(self._grabador)
        self._grabador.recorderStateChanged.connect(self._on_grabador_estado)
        self._grabador.errorOccurred.connect(
            lambda err, cad: QMessageBox.critical(self, "Error de grabacion", cad)
        )
        return True

    def _grabando(self) -> bool:
        return (self._grabador is not None
                and self._grabador.recorderState() == QMediaRecorder.RecorderState.RecordingState)

    def _toggle_grabacion(self):
        """Arranca o detiene la grabacion del microfono."""
        if not self._init_grabador():
            return
        if self._grabando():
            self._grabador.stop()
        else:
            self._detener_reproduccion()
            destino = Path(tempfile.gettempdir()) / "grabacion_transcriptor"
            self._grabador.setOutputLocation(QUrl.fromLocalFile(str(destino)))
            self._grabador.record()

    def _on_grabador_estado(self, estado):
        if estado == QMediaRecorder.RecorderState.RecordingState:
            self.btn_grabar.setText("■ Detener")
            self.btn_cargar.setEnabled(False)
            self.btn_transcribir.setEnabled(False)
            self._set_estado("Grabando... apretá Detener cuando termines.")
        elif estado == QMediaRecorder.RecorderState.StoppedState:
            self.btn_grabar.setText("● Grabar")
            self.btn_cargar.setEnabled(True)
            ruta = self._grabador.actualLocation().toLocalFile()
            if ruta and Path(ruta).exists():
                self.ruta_audio = ruta
                self.lbl_archivo.setText(f"{Path(ruta).name} (grabado)")
                self.lbl_archivo.setStyleSheet("color: #111;")
                self.btn_transcribir.setEnabled(True)
                self.btn_reproducir.setEnabled(True)
                self._set_estado(f"Grabacion lista: {Path(ruta).name}")
            else:
                self._set_estado("No se pudo guardar la grabacion.")

    # ------------------------------------------------------------------ reproduccion
    def _init_player(self):
        """Inicializa el reproductor la primera vez que se usa."""
        if self._player is not None:
            return
        self._audio_output = QAudioOutput()
        self._player = QMediaPlayer()
        self._player.setAudioOutput(self._audio_output)
        self._player.playbackStateChanged.connect(self._on_player_estado)

    def _toggle_reproduccion(self):
        """Reproduce el audio cargado/grabado, o lo detiene si ya esta sonando."""
        if not self.ruta_audio:
            return
        self._init_player()
        if self._player.playbackState() == QMediaPlayer.PlaybackState.PlayingState:
            self._player.stop()
        else:
            self._player.setSource(QUrl.fromLocalFile(self.ruta_audio))
            self._player.play()

    def _on_player_estado(self, estado):
        if estado == QMediaPlayer.PlaybackState.PlayingState:
            self.btn_reproducir.setText("■ Detener")
        else:  # Paused o Stopped
            self.btn_reproducir.setText("▶ Reproducir")

    def _detener_reproduccion(self):
        """Frena la reproduccion si esta sonando (al cargar otro audio o transcribir)."""
        if self._player is not None:
            self._player.stop()

    def _aplicar_estilo(self):
        self.setStyleSheet("""
            QPushButton {
                background-color: #2563eb;
                color: white;
                border-radius: 6px;
                padding: 6px 14px;
                font-weight: 600;
            }
            QPushButton:disabled { background-color: #94a3b8; }
            QPushButton:hover:!disabled { background-color: #1d4ed8; }
            QTabWidget::pane { border: 1px solid #cbd5e1; }
            QTextEdit { border: 1px solid #cbd5e1; padding: 6px; }
            QLineEdit { border: 1px solid #cbd5e1; padding: 4px 6px; border-radius: 4px; }
            QProgressBar { border: 1px solid #cbd5e1; border-radius: 4px; text-align: center; }
            QProgressBar::chunk { background-color: #2563eb; }
        """)

    # ------------------------------------------------------------------ AI
    def _cargar_ia_si_corresponde(self):
        api_key = os.getenv("OPENAI_API_KEY", "")
        if not api_key:
            self._set_estado("Sin OPENAI_API_KEY en .env -- funciones IA deshabilitadas")
            return
        try:
            self.ia = AsistenteIA(
                api_key=api_key,
                modelo=os.getenv("OPENAI_MODEL", "gemini-2.5-flash"),
                base_url=os.getenv("OPENAI_BASE_URL", "https://generativelanguage.googleapis.com/v1beta/openai/"),
            )
        except Exception as e:
            QMessageBox.warning(self, "Aviso", f"No se pudo inicializar la IA: {e}")

    # ------------------------------------------------------------------ acciones
    def _cargar_audio(self):
        ruta, _ = QFileDialog.getOpenFileName(
            self,
            "Seleccionar archivo de audio",
            str(Path.home()),
            "Audio (*.mp3 *.wav *.m4a *.flac *.ogg *.aac);;Todos los archivos (*.*)",
        )
        if not ruta:
            return
        self._detener_reproduccion()
        self.ruta_audio = ruta
        self.lbl_archivo.setText(Path(ruta).name)
        self.lbl_archivo.setStyleSheet("color: #111;")
        self.btn_transcribir.setEnabled(True)
        self.btn_reproducir.setEnabled(True)
        self._set_estado(f"Archivo cargado: {Path(ruta).name}")

    def _transcribir(self):
        if not self.ruta_audio:
            return
        if self._grabando():
            QMessageBox.information(self, "Grabando",
                                    "Detené la grabacion antes de transcribir.")
            return

        hf_token = os.getenv("HF_TOKEN", "")
        if not hf_token:
            QMessageBox.warning(
                self,
                "Sin token HF",
                "No hay HF_TOKEN en .env. La transcripcion va a salir SIN identificar hablantes.\n"
                "Generá uno gratis en https://huggingface.co/settings/tokens",
            )

        # Lazy import: torch + whisperx pesan varios GB y tardan minutos en cargar.
        # Solo los importamos cuando el usuario realmente va a transcribir.
        self._set_estado("Cargando librerias de ML... (primera vez puede tardar)")
        QApplication.processEvents()
        import torch
        from src.pipeline import PipelineDeTranscripcion

        dispositivo = "cuda" if torch.cuda.is_available() else "cpu"
        num_personas = self.spin_personas.value()  # la fija el usuario (siempre exacto)

        self.pipeline = PipelineDeTranscripcion(
            modelo_whisper=self.combo_modelo.currentData(),
            idioma=self.combo_idioma.currentText(),
            hf_token=hf_token,
            dispositivo=dispositivo,
            num_hablantes=num_personas,
        )

        self._desactivar_botones(True)
        self.progress.setValue(0)
        self._set_estado("Procesando audio... (puede tardar varios minutos)")

        self.worker = TareaWorker(self.pipeline.ejecutar, self.ruta_audio)
        # Inyectar callback de progreso ANTES del start para que llegue a la barra
        self.pipeline.progreso = lambda etapa, pct: self.worker.progreso.emit(etapa, pct)
        self.worker.progreso.connect(self._on_progreso)
        self.worker.terminado.connect(self._on_transcripcion_lista)
        self.worker.error.connect(self._on_error)
        self.worker.start()

    def _on_progreso(self, etapa: str, pct: int):
        self.progress.setValue(pct)
        self._set_estado(etapa)

    def _on_transcripcion_lista(self, salida: dict):
        self.texto_original = salida["texto"]
        self.txt_original.setPlainText(self.texto_original)

        participacion = salida.get("participacion", {})
        if participacion:
            partes = " | ".join(f"{n}: {c}" for n, c in participacion.items())
            self.lbl_participacion.setText(f"Participacion: {partes}")
        else:
            self.lbl_participacion.setText("")

        self._desactivar_botones(False)
        self.btn_guardar.setEnabled(True)
        if self.ia:
            self.btn_limpiar.setEnabled(True)
            self.btn_resumir.setEnabled(True)
            self.btn_analizar.setEnabled(True)
        self.progress.setValue(100)
        self._set_estado("Transcripcion lista")
        self._liberar_pipeline()

    def _on_error(self, msg: str):
        self._desactivar_botones(False)
        self.progress.setValue(0)
        self._set_estado("Error en el procesamiento")
        self._liberar_pipeline()
        QMessageBox.critical(self, "Error", msg)

    def _liberar_pipeline(self):
        """
        Vacia por completo la memoria del pipeline tras cada audio: suelta los
        modelos y limpia la VRAM. Asi corridas sucesivas no acumulan memoria
        (evita el CUDA out of memory al transcribir varios audios seguidos).
        """
        try:
            if self.pipeline is not None:
                self.pipeline.liberar_todo()
        except Exception:
            pass
        self.pipeline = None
        import gc
        gc.collect()
        try:
            import torch
            if torch.cuda.is_available():
                torch.cuda.empty_cache()
                torch.cuda.ipc_collect()
        except Exception:
            pass

    def _desactivar_botones(self, procesando: bool):
        self.btn_transcribir.setEnabled(not procesando and bool(self.ruta_audio))
        self.btn_cargar.setEnabled(not procesando)
        self.btn_grabar.setEnabled(not procesando)
        self.btn_reproducir.setEnabled(not procesando and bool(self.ruta_audio))
        if procesando:
            self._detener_reproduccion()
            self.btn_limpiar.setEnabled(False)
            self.btn_resumir.setEnabled(False)
            self.btn_analizar.setEnabled(False)
            self.btn_guardar.setEnabled(False)

    # ----- IA actions -----
    def _accion_ia(self, metodo, area_destino: QTextEdit, etiqueta: str):
        if not self.ia or not self.texto_original:
            return
        area_destino.setPlainText(f"Procesando '{etiqueta}'...")
        self._set_estado(f"Llamando a OpenAI para '{etiqueta}'...")
        self.worker = TareaWorker(metodo, self.texto_original)
        self.worker.terminado.connect(lambda txt: self._on_ia_listo(txt, area_destino, etiqueta))
        self.worker.error.connect(self._on_error)
        self.worker.start()

    def _on_ia_listo(self, texto: str, area: QTextEdit, etiqueta: str):
        area.setPlainText(texto)
        self._set_estado(f"{etiqueta} listo")

    def _limpiar_texto(self):
        self._accion_ia(self.ia.limpiar, self.txt_limpio, "Limpieza")

    def _resumir(self):
        self._accion_ia(self.ia.resumir, self.txt_resumen, "Resumen")

    def _analizar(self):
        self._accion_ia(self.ia.analizar, self.txt_analisis, "Analisis")

    def _preguntar(self):
        if not self.ia or not self.texto_original:
            QMessageBox.information(self, "Sin contexto", "Primero transcribi un audio.")
            return
        pregunta = self.input_pregunta.text().strip()
        if not pregunta:
            return
        self.input_pregunta.clear()
        self.txt_chat.append(f"\n<b>Vos:</b> {pregunta}")
        self.txt_chat.append("<i>Pensando...</i>")
        self._set_estado("Consultando a OpenAI...")
        self.btn_preguntar.setEnabled(False)

        self.worker = TareaWorker(
            self.ia.preguntar,
            self.texto_original,
            pregunta,
            historial=list(self.historial_chat),
        )
        self.worker.terminado.connect(lambda r: self._on_respuesta(pregunta, r))
        self.worker.error.connect(self._on_error_chat)
        self.worker.start()

    def _on_respuesta(self, pregunta: str, respuesta: str):
        # Reemplazar "Pensando..." con la respuesta
        contenido = self.txt_chat.toHtml()
        contenido = contenido.replace("<i>Pensando...</i>", "")
        self.txt_chat.setHtml(contenido)
        self.txt_chat.append(f"<b>IA:</b> {respuesta}\n")
        self.historial_chat.append((pregunta, respuesta))
        self.btn_preguntar.setEnabled(True)
        self._set_estado("Listo")

    def _on_error_chat(self, msg: str):
        self.btn_preguntar.setEnabled(True)
        self._on_error(msg)

    # ----- guardar -----
    def _guardar_todo(self):
        if not self.ruta_audio:
            return
        base = Path(self.ruta_audio).with_suffix("")
        guardados = []
        if self.texto_original:
            p = Path(f"{base}_transcripcion.txt"); p.write_text(self.texto_original, encoding="utf-8"); guardados.append(p.name)
        if self.txt_limpio.toPlainText().strip():
            p = Path(f"{base}_limpio.txt"); p.write_text(self.txt_limpio.toPlainText(), encoding="utf-8"); guardados.append(p.name)
        if self.txt_resumen.toPlainText().strip():
            p = Path(f"{base}_resumen.txt"); p.write_text(self.txt_resumen.toPlainText(), encoding="utf-8"); guardados.append(p.name)
        if self.txt_analisis.toPlainText().strip():
            p = Path(f"{base}_analisis.txt"); p.write_text(self.txt_analisis.toPlainText(), encoding="utf-8"); guardados.append(p.name)
        if guardados:
            QMessageBox.information(self, "Guardado", "Archivos generados:\n" + "\n".join(guardados))

    # ----- utilidades -----
    def _set_estado(self, msg: str):
        self.statusBar().showMessage(msg)
