#include "gui/VentanaPrincipal.h"

#include <QAudioDevice>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QMediaDevices>
#include <QMessageBox>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStringConverter>
#include <QStyle>
#include <QTextStream>
#include <QUrl>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrent>

namespace DIArize::GUI {

// ════════════════════════════════════════════════════════════════════
//   Constructor
// ════════════════════════════════════════════════════════════════════
VentanaPrincipal::VentanaPrincipal(QWidget* parent)
    : QMainWindow(parent)
    , _transcriptor(std::make_unique<DIArize::Red::ClienteTranscriptor>())
{
    setWindowTitle("DIArize — Transcriptor de Audio con IA");
    resize(1100, 760);

    _construirUI();

    // Workers asincronicos
    _watcherTranscribir = new QFutureWatcher<DIArize::Core::Transcripcion>(this);
    _watcherLimpiar     = new QFutureWatcher<DIArize::Red::RespuestaIA>(this);
    _watcherResumir     = new QFutureWatcher<DIArize::Red::RespuestaIA>(this);
    _watcherAnalizar    = new QFutureWatcher<DIArize::Red::RespuestaIA>(this);
    _watcherTraducir    = new QFutureWatcher<DIArize::Red::RespuestaIA>(this);
    _watcherPreguntar   = new QFutureWatcher<DIArize::Red::RespuestaIA>(this);

    connect(_watcherTranscribir, &QFutureWatcher<DIArize::Core::Transcripcion>::finished,
            this, &VentanaPrincipal::_onTranscripcionLista);
    connect(_watcherLimpiar,   &QFutureWatcher<DIArize::Red::RespuestaIA>::finished,
            this, &VentanaPrincipal::_onLimpiarListo);
    connect(_watcherResumir,   &QFutureWatcher<DIArize::Red::RespuestaIA>::finished,
            this, &VentanaPrincipal::_onResumirListo);
    connect(_watcherAnalizar,  &QFutureWatcher<DIArize::Red::RespuestaIA>::finished,
            this, &VentanaPrincipal::_onAnalizarListo);
    connect(_watcherTraducir,  &QFutureWatcher<DIArize::Red::RespuestaIA>::finished,
            this, &VentanaPrincipal::_onTraducirListo);
    connect(_watcherPreguntar, &QFutureWatcher<DIArize::Red::RespuestaIA>::finished,
            this, &VentanaPrincipal::_onPreguntarListo);

    // Timer para contar segundos durante la grabacion
    _timerRec = new QTimer(this);
    _timerRec->setInterval(1000);
    connect(_timerRec, &QTimer::timeout, this, &VentanaPrincipal::_tickGrabacion);

    // Arranca el servidor Flask automaticamente
    _iniciarServidor();
}

VentanaPrincipal::~VentanaPrincipal() = default;

// ════════════════════════════════════════════════════════════════════
//   UI
// ════════════════════════════════════════════════════════════════════
void VentanaPrincipal::_construirUI() {
    _panelEnVivo = new PanelEnVivo(this);
    connect(_panelEnVivo, &PanelEnVivo::procesarConArchivo,
            this, &VentanaPrincipal::_procesarWavDeEnVivo);

    auto* central = new QWidget(this);
    central->setObjectName("centralBg");
    auto* vlay    = new QVBoxLayout(central);
    vlay->setContentsMargins(14, 14, 14, 14);
    vlay->setSpacing(12);

    // Toggle de modo arriba
    vlay->addWidget(_construirToggleModo());

    // Stack: index 0 = modo archivo (UI actual), index 1 = modo en vivo
    _stack = new QStackedWidget(this);
    _stack->addWidget(_construirModoArchivo());
    _stack->addWidget(_panelEnVivo);
    vlay->addWidget(_stack, 1);

    setCentralWidget(central);

    statusBar()->showMessage("Iniciando servidor...");
}

QWidget* VentanaPrincipal::_construirToggleModo() {
    auto* w = new QWidget(this);
    w->setObjectName("toggleContainer");
    auto* lay = new QHBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    _btnModoArchivo = new QPushButton("📁  Desde archivo", this);
    _btnModoEnVivo  = new QPushButton("🔴  EN VIVO",       this);
    _btnModoArchivo->setObjectName("btnToggleActivo");
    _btnModoEnVivo ->setObjectName("btnToggleInactivo");
    _btnModoArchivo->setCheckable(true);
    _btnModoEnVivo ->setCheckable(true);
    _btnModoArchivo->setChecked(true);
    _btnModoArchivo->setMinimumHeight(42);
    _btnModoEnVivo ->setMinimumHeight(42);

    lay->addWidget(_btnModoArchivo);
    lay->addSpacing(8);
    lay->addWidget(_btnModoEnVivo);
    lay->addStretch(1);

    connect(_btnModoArchivo, &QPushButton::clicked, this, &VentanaPrincipal::_cambiarAModoArchivo);
    connect(_btnModoEnVivo,  &QPushButton::clicked, this, &VentanaPrincipal::_cambiarAModoEnVivo);
    return w;
}

void VentanaPrincipal::_cambiarAModoArchivo() {
    _stack->setCurrentIndex(0);
    _btnModoArchivo->setChecked(true);
    _btnModoEnVivo ->setChecked(false);
    _btnModoArchivo->setObjectName("btnToggleActivo");
    _btnModoEnVivo ->setObjectName("btnToggleInactivo");
    _btnModoArchivo->style()->unpolish(_btnModoArchivo);
    _btnModoArchivo->style()->polish(_btnModoArchivo);
    _btnModoEnVivo ->style()->unpolish(_btnModoEnVivo);
    _btnModoEnVivo ->style()->polish(_btnModoEnVivo);
}

void VentanaPrincipal::_cambiarAModoEnVivo() {
    _stack->setCurrentIndex(1);
    _btnModoArchivo->setChecked(false);
    _btnModoEnVivo ->setChecked(true);
    _btnModoArchivo->setObjectName("btnToggleInactivo");
    _btnModoEnVivo ->setObjectName("btnToggleActivo");
    _btnModoArchivo->style()->unpolish(_btnModoArchivo);
    _btnModoArchivo->style()->polish(_btnModoArchivo);
    _btnModoEnVivo ->style()->unpolish(_btnModoEnVivo);
    _btnModoEnVivo ->style()->polish(_btnModoEnVivo);
}

void VentanaPrincipal::_procesarWavDeEnVivo(const QString& rutaWav) {
    // Cambio a modo Archivo, precargo el WAV grabado durante el streaming
    // y disparo Transcribir automaticamente para que el usuario vea el resultado
    // diarizado sin tener que apretar nada mas.
    _cambiarAModoArchivo();
    _setRutaAudio(rutaWav, "Grabacion en vivo (" + QFileInfo(rutaWav).fileName() + ")");
    statusBar()->showMessage("Procesando grabacion en vivo con diarizacion...");
    _transcribir();
}

QWidget* VentanaPrincipal::_construirModoArchivo() {
    // ── Toolbar ───────────────────────────────────────────────────
    _btnCargar    = new QPushButton("⬆  Cargar audio", this);
    _btnCargar->setObjectName("btnCargar");
    _btnGrabar    = new QPushButton("● Grabar", this);
    _btnEscuchar  = new QPushButton("▶ Escuchar", this);
    _lblArchivo   = new QLabel("Ningun archivo seleccionado", this);
    _btnQuitar    = new QPushButton("✕", this);
    _comboIdioma  = new QComboBox(this);
    _spinPersonas = new QSpinBox(this);

    _btnGrabar  ->setObjectName("btnGrabar");
    _btnEscuchar->setObjectName("btnEscuchar");
    _btnQuitar  ->setObjectName("btnQuitar");
    _lblArchivo ->setObjectName("lblArchivo");

    _comboIdioma->addItems({"es", "en", "pt", "fr", "it", "de"});
    _spinPersonas->setRange(0, 10);
    _spinPersonas->setValue(0);
    _spinPersonas->setSpecialValueText("Auto");
    _spinPersonas->setToolTip(
        "Cantidad de personas que hablan en el audio.\n"
        "0 = Auto (deja que AssemblyAI decida).");

    _btnCargar  ->setEnabled(false);
    _btnGrabar  ->setEnabled(false);
    _btnEscuchar->setEnabled(false);
    _btnQuitar  ->setVisible(false);

    auto* toolbarBox = new QHBoxLayout();
    toolbarBox->setSpacing(8);
    toolbarBox->addWidget(_btnCargar);
    toolbarBox->addWidget(_btnGrabar);
    toolbarBox->addWidget(_btnEscuchar);
    toolbarBox->addWidget(_lblArchivo, 1);
    toolbarBox->addWidget(_btnQuitar);
    toolbarBox->addSpacing(10);
    toolbarBox->addWidget(new QLabel("Idioma:", this));
    toolbarBox->addWidget(_comboIdioma);
    toolbarBox->addSpacing(6);
    toolbarBox->addWidget(new QLabel("Personas:", this));
    toolbarBox->addWidget(_spinPersonas);

    // ── Accion: Transcribir + progreso ────────────────────────────
    _btnTranscribir = new QPushButton("Transcribir audio", this);
    _btnTranscribir->setObjectName("btnTranscribir");
    _btnTranscribir->setMinimumHeight(38);
    _btnTranscribir->setEnabled(false);

    _progress = new QProgressBar(this);
    _progress->setRange(0, 100);
    _progress->setValue(0);
    _progress->setTextVisible(false);

    auto* accionBox = new QHBoxLayout();
    accionBox->setSpacing(10);
    accionBox->addWidget(_btnTranscribir);
    accionBox->addWidget(_progress, 1);

    // ── Tabs ──────────────────────────────────────────────────────
    _tabs = new QTabWidget(this);

    _areaOriginal = new QTextEdit(this);
    _areaOriginal->setReadOnly(true);
    _areaOriginal->setPlaceholderText("La transcripcion con timestamps y personas aparece aca...");
    _tabs->addTab(_areaOriginal, "Original");

    _tabs->addTab(_construirTabIA(_areaLimpio,   _btnLimpiar,
                                   "Limpiar con IA",
                                   "Texto limpio (sin muletillas) generado por IA..."),
                  "Limpio");
    _tabs->addTab(_construirTabIA(_areaResumen,  _btnResumir,
                                   "Resumir con IA",
                                   "Resumen generado por IA..."),
                  "Resumen");
    _tabs->addTab(_construirTabIA(_areaAnalisis, _btnAnalizar,
                                   "Analizar con IA",
                                   "Analisis (temas, tono, acciones) generado por IA..."),
                  "Analisis");

    // Tab Traducir — combo de idioma + boton + area
    {
        auto* w   = new QWidget(this);
        auto* lay = new QVBoxLayout(w);
        lay->setContentsMargins(0, 8, 0, 0);
        lay->setSpacing(8);

        auto* topBox = new QHBoxLayout();
        topBox->addWidget(new QLabel("Idioma destino:", w));
        _comboIdiomaTraducir = new QComboBox(w);
        _comboIdiomaTraducir->addItem("Ingles",     "en");
        _comboIdiomaTraducir->addItem("Espanol",    "es");
        _comboIdiomaTraducir->addItem("Portugues",  "pt");
        _comboIdiomaTraducir->addItem("Frances",    "fr");
        _comboIdiomaTraducir->addItem("Italiano",   "it");
        _comboIdiomaTraducir->addItem("Aleman",     "de");
        _comboIdiomaTraducir->addItem("Japones",    "ja");
        _comboIdiomaTraducir->addItem("Chino",      "zh");
        _comboIdiomaTraducir->addItem("Coreano",    "ko");
        _comboIdiomaTraducir->addItem("Ruso",       "ru");
        _comboIdiomaTraducir->setMinimumWidth(140);
        topBox->addWidget(_comboIdiomaTraducir);
        topBox->addStretch(1);

        _btnTraducir = new QPushButton("Traducir con IA", w);
        _btnTraducir->setObjectName("btnIA");
        _btnTraducir->setEnabled(false);
        topBox->addWidget(_btnTraducir);
        lay->addLayout(topBox);

        _areaTraducido = new QTextEdit(w);
        _areaTraducido->setReadOnly(true);
        _areaTraducido->setPlaceholderText(
            "La traduccion al idioma seleccionado va a aparecer aca...");
        lay->addWidget(_areaTraducido, 1);

        _tabs->addTab(w, "Traducir");
    }

    _tabs->addTab(_construirTabChat(), "Chat (Q&A)");

    // ── Bottom: participacion + guardar ───────────────────────────
    _lblParticipacion = new QLabel("", this);
    _lblParticipacion->setObjectName("lblParticipacion");
    _btnGuardar       = new QPushButton("Guardar todo (.txt)", this);
    _btnGuardar->setEnabled(false);

    auto* bottomBox = new QHBoxLayout();
    bottomBox->addWidget(_lblParticipacion, 1);
    bottomBox->addWidget(_btnGuardar);

    // ── Layout del modo Archivo ───────────────────────────────────
    auto* modoArchivo = new QWidget(this);
    auto* vlayMA      = new QVBoxLayout(modoArchivo);
    vlayMA->setContentsMargins(0, 0, 0, 0);
    vlayMA->setSpacing(10);
    vlayMA->addLayout(toolbarBox);
    vlayMA->addLayout(accionBox);
    vlayMA->addWidget(_tabs, 1);
    vlayMA->addLayout(bottomBox);

    // ── Signals & slots ───────────────────────────────────────────
    connect(_btnCargar,      &QPushButton::clicked, this, &VentanaPrincipal::_seleccionarArchivo);
    connect(_btnQuitar,      &QPushButton::clicked, this, &VentanaPrincipal::_quitarAudio);
    connect(_btnGrabar,      &QPushButton::clicked, this, &VentanaPrincipal::_toggleGrabacion);
    connect(_btnEscuchar,    &QPushButton::clicked, this, &VentanaPrincipal::_toggleReproduccion);
    connect(_btnTranscribir, &QPushButton::clicked, this, &VentanaPrincipal::_transcribir);
    connect(_btnLimpiar,     &QPushButton::clicked, this, &VentanaPrincipal::_limpiar);
    connect(_btnResumir,     &QPushButton::clicked, this, &VentanaPrincipal::_resumir);
    connect(_btnAnalizar,    &QPushButton::clicked, this, &VentanaPrincipal::_analizar);
    connect(_btnTraducir,    &QPushButton::clicked, this, &VentanaPrincipal::_traducir);
    connect(_btnGuardar,     &QPushButton::clicked, this, &VentanaPrincipal::_guardarTodo);

    return modoArchivo;
}

QWidget* VentanaPrincipal::_construirTabIA(QTextEdit*& areaSalida,
                                            QPushButton*& boton,
                                            const QString& etiquetaBoton,
                                            const QString& placeholder)
{
    auto* w   = new QWidget(this);
    auto* lay = new QVBoxLayout(w);
    lay->setContentsMargins(0, 8, 0, 0);
    lay->setSpacing(8);

    boton = new QPushButton(etiquetaBoton, w);
    boton->setObjectName("btnIA");
    boton->setEnabled(false);
    lay->addWidget(boton);

    areaSalida = new QTextEdit(w);
    areaSalida->setReadOnly(true);
    areaSalida->setPlaceholderText(placeholder);
    lay->addWidget(areaSalida, 1);
    return w;
}

QWidget* VentanaPrincipal::_construirTabChat() {
    auto* w   = new QWidget(this);
    auto* lay = new QVBoxLayout(w);
    lay->setContentsMargins(0, 8, 0, 0);
    lay->setSpacing(8);

    _areaChat = new QTextEdit(w);
    _areaChat->setReadOnly(true);
    _areaChat->setPlaceholderText("Hacele preguntas sobre lo que se dijo en el audio.");
    lay->addWidget(_areaChat, 1);

    auto* inputBox  = new QHBoxLayout();
    _inputPregunta  = new QLineEdit(w);
    _inputPregunta->setPlaceholderText("Escribi tu pregunta y enter...");
    _btnPreguntar   = new QPushButton("Preguntar", w);
    _btnPreguntar->setObjectName("btnIA");
    inputBox->addWidget(_inputPregunta, 1);
    inputBox->addWidget(_btnPreguntar);
    lay->addLayout(inputBox);

    connect(_inputPregunta, &QLineEdit::returnPressed, this, &VentanaPrincipal::_preguntar);
    connect(_btnPreguntar,  &QPushButton::clicked,     this, &VentanaPrincipal::_preguntar);
    return w;
}

// ════════════════════════════════════════════════════════════════════
//   Inicio automatico del servidor
// ════════════════════════════════════════════════════════════════════
void VentanaPrincipal::_iniciarServidor() {
    QString appDir = QCoreApplication::applicationDirPath();
    _serverProcess = new QProcess(this);

    // 1) Si hay servidor.exe al lado (deploy con PyInstaller): usar eso
    QString exePath = appDir + "/servidor.exe";
    if (QFile::exists(exePath)) {
        _serverProcess->setWorkingDirectory(appDir);
        _serverProcess->start(exePath, {});
    } else {
        // 2) Desarrollo: buscar el venv/servidor.py subiendo desde appDir.
        //    Busca un ancestro que tenga "venv/Scripts/pythonw.exe" y "Transcriptor/servidor.py".
        QDir dir(appDir);
        QString python, server;
        for (int i = 0; i < 8 && python.isEmpty(); ++i) {
            QString cand_py  = dir.absoluteFilePath("venv/Scripts/pythonw.exe");
            QString cand_srv = dir.absoluteFilePath("Transcriptor/servidor.py");
            if (QFile::exists(cand_py) && QFile::exists(cand_srv)) {
                python = cand_py;
                server = cand_srv;
                break;
            }
            if (!dir.cdUp()) break;
        }

        if (python.isEmpty()) {
            // Ultima opcion: pythonw del PATH (asume que el server.py esta al lado del exe)
            python = "pythonw";
            server = appDir + "/servidor.py";
        }

        if (QFile::exists(server)) {
            _serverProcess->setWorkingDirectory(QFileInfo(server).absolutePath());
            _serverProcess->start(python, { server });
        }
    }

    _startupTimer = new QTimer(this);
    connect(_startupTimer, &QTimer::timeout,
            this, &VentanaPrincipal::_verificarServidor);
    _startupTimer->start(1000);
}

void VentanaPrincipal::_verificarServidor() {
    _startupAttempts++;

    if (_transcriptor->estaDisponible()) {
        _startupTimer->stop();
        _btnCargar  ->setEnabled(true);
        _btnGrabar  ->setEnabled(true);
        statusBar()->showMessage("Listo. Carga un archivo o grabá audio para empezar.");
        return;
    }

    if (_startupAttempts >= 30) {
        _startupTimer->stop();
        statusBar()->showMessage("Error: no se pudo conectar al servidor.");
        QMessageBox::critical(this, "Error de inicio",
            "El servidor no respondio despues de 30 segundos.\n\n"
            "Asegurate de que servidor.exe este en la misma carpeta que DIArizeCpp.exe.");
        return;
    }

    statusBar()->showMessage(
        QString("Iniciando servidor...  %1s").arg(_startupAttempts));
}

// ════════════════════════════════════════════════════════════════════
//   closeEvent: apaga el servidor
// ════════════════════════════════════════════════════════════════════
void VentanaPrincipal::closeEvent(QCloseEvent* e) {
    if (_startupTimer) _startupTimer->stop();
    if (_player) _player->stop();
    if (_grabador && _grabador->recorderState() == QMediaRecorder::RecordingState)
        _grabador->stop();
    if (_serverProcess && _serverProcess->state() != QProcess::NotRunning) {
        _serverProcess->terminate();
        _serverProcess->waitForFinished(3000);
        if (_serverProcess->state() != QProcess::NotRunning)
            _serverProcess->kill();
    }
    QMainWindow::closeEvent(e);
}

// ════════════════════════════════════════════════════════════════════
//   IObservador
// ════════════════════════════════════════════════════════════════════
void VentanaPrincipal::onEstadoCambiado(DIArize::Core::EstadoApp estado,
                                         const std::string& detalle)
{
    using E = DIArize::Core::EstadoApp;
    if (estado == E::Procesando) _progress->setRange(0, 0);
    else                         _progress->setRange(0, 100);
    statusBar()->showMessage(QString::fromStdString(detalle));
}

void VentanaPrincipal::onProgreso(int porcentaje, const std::string& etapa) {
    if (_progress->maximum() != 0) _progress->setValue(porcentaje);
    statusBar()->showMessage(QString::fromStdString(etapa));
}

// ════════════════════════════════════════════════════════════════════
//   Archivo
// ════════════════════════════════════════════════════════════════════
void VentanaPrincipal::_setRutaAudio(const QString& ruta, const QString& displayName) {
    _rutaAudio = ruta;
    _lblArchivo->setText(displayName);
    _lblArchivo->setStyleSheet("color: #1f2937; font-weight: 500;");
    _btnTranscribir->setEnabled(true);
    _btnEscuchar->setEnabled(true);
    _btnQuitar->setVisible(true);
}

void VentanaPrincipal::_seleccionarArchivo() {
    if (_player) _player->stop();
    QString ruta = QFileDialog::getOpenFileName(
        this, "Seleccionar audio", QDir::homePath(),
        "Audio (*.mp3 *.wav *.m4a *.flac *.ogg);;Todos (*.*)");
    if (ruta.isEmpty()) return;
    _setRutaAudio(ruta, QFileInfo(ruta).fileName());
    statusBar()->showMessage("Archivo cargado: " + QFileInfo(ruta).fileName());
}

void VentanaPrincipal::_quitarAudio() {
    if (_player) _player->stop();
    _rutaAudio.clear();
    _lblArchivo->setText("Ningun archivo seleccionado");
    _lblArchivo->setStyleSheet("");
    _btnTranscribir->setEnabled(false);
    _btnEscuchar->setEnabled(false);
    _btnQuitar->setVisible(false);
    statusBar()->showMessage("Audio quitado. Carga otro o grabá uno nuevo.");
}

// ════════════════════════════════════════════════════════════════════
//   Grabacion
// ════════════════════════════════════════════════════════════════════
void VentanaPrincipal::_toggleGrabacion() {
    if (_grabador && _grabador->recorderState() == QMediaRecorder::RecordingState) {
        // Detener
        _timerRec->stop();
        _grabador->stop();

        QString ruta = _grabador->actualLocation().toLocalFile();
        if (!ruta.isEmpty() && QFile::exists(ruta)) {
            int m = _segGrabando / 60, s = _segGrabando % 60;
            _setRutaAudio(ruta, QString("Grabacion (%1:%2)")
                                    .arg(m, 2, 10, QLatin1Char('0'))
                                    .arg(s, 2, 10, QLatin1Char('0')));
            statusBar()->showMessage("Grabacion lista. Podes escuchar o transcribir.");
        } else {
            statusBar()->showMessage("Grabacion vacia. Intenta de nuevo.");
        }

        _btnGrabar->setText("● Grabar");
        _btnCargar->setEnabled(true);
        return;
    }

    // Iniciar
    if (QMediaDevices::audioInputs().isEmpty()) {
        QMessageBox::warning(this, "Sin microfono",
                             "No se detecto ningun microfono en el sistema.");
        return;
    }
    if (!_grabador) {
        _captura    = new QMediaCaptureSession(this);
        _audioInput = new QAudioInput(this);
        _grabador   = new QMediaRecorder(this);
        _captura->setAudioInput(_audioInput);
        _captura->setRecorder(_grabador);
    }

    QString carpetaTmp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString nombre = QString("diarize_rec_%1.m4a")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    _grabador->setOutputLocation(QUrl::fromLocalFile(carpetaTmp + "/" + nombre));
    _grabador->record();

    _segGrabando = 0;
    _timerRec->start();
    _btnGrabar->setText("■ Detener");
    _btnCargar->setEnabled(false);
    _btnTranscribir->setEnabled(false);
    _btnEscuchar->setEnabled(false);
    _btnQuitar->setVisible(false);
    _lblArchivo->setText("● Grabando  00:00");
    _lblArchivo->setStyleSheet("color: #dc2626; font-weight: bold;");
    statusBar()->showMessage("Grabando... presiona Detener cuando termines.");
}

void VentanaPrincipal::_tickGrabacion() {
    _segGrabando++;
    int m = _segGrabando / 60, s = _segGrabando % 60;
    _lblArchivo->setText(QString("● Grabando  %1:%2")
                            .arg(m, 2, 10, QLatin1Char('0'))
                            .arg(s, 2, 10, QLatin1Char('0')));
}

// ════════════════════════════════════════════════════════════════════
//   Reproduccion
// ════════════════════════════════════════════════════════════════════
void VentanaPrincipal::_toggleReproduccion() {
    if (_rutaAudio.isEmpty()) return;

    if (!_player) {
        _player      = new QMediaPlayer(this);
        _audioOutput = new QAudioOutput(this);
        _player->setAudioOutput(_audioOutput);
        _audioOutput->setVolume(1.0);
        connect(_player, &QMediaPlayer::playbackStateChanged,
                this, [this](QMediaPlayer::PlaybackState st) {
            _btnEscuchar->setText(
                st == QMediaPlayer::PlayingState ? "■ Detener" : "▶ Escuchar");
        });
    }

    if (_player->playbackState() == QMediaPlayer::PlayingState) {
        _player->stop();
    } else {
        _player->setSource(QUrl::fromLocalFile(_rutaAudio));
        _player->play();
    }
}

// ════════════════════════════════════════════════════════════════════
//   Transcripcion (async)
// ════════════════════════════════════════════════════════════════════
void VentanaPrincipal::_transcribir() {
    if (_rutaAudio.isEmpty()) return;
    if (_player) _player->stop();

    _btnTranscribir->setEnabled(false);
    _btnCargar     ->setEnabled(false);
    _btnGrabar     ->setEnabled(false);
    _btnEscuchar   ->setEnabled(false);
    _habilitarBotonesIA(false);
    _progress->setRange(0, 0);
    statusBar()->showMessage("Enviando audio al servidor...");

    QString  ruta    = _rutaAudio;
    QString  idioma  = _comboIdioma->currentText();
    int      personas = _spinPersonas->value();
    auto*    cli     = _transcriptor.get();

    auto fut = QtConcurrent::run([cli, ruta, idioma, personas]() {
        return cli->transcribir(ruta.toStdString(), idioma.toStdString(), personas);
    });
    _watcherTranscribir->setFuture(fut);
}

QString VentanaPrincipal::_formatoOriginal() const {
    return _textoOriginal;
}

void VentanaPrincipal::_onTranscripcionLista() {
    DIArize::Core::Transcripcion t = _watcherTranscribir->result();

    _progress->setRange(0, 100);
    _btnCargar     ->setEnabled(true);
    _btnGrabar     ->setEnabled(true);
    _btnEscuchar   ->setEnabled(true);
    _btnTranscribir->setEnabled(true);

    if (t.getSegmentos().empty() && t.getTexto().empty()) {
        _progress->setValue(0);
        statusBar()->showMessage("Error en la transcripcion.");
        QMessageBox::warning(this, "Error",
            "No se pudo obtener una transcripcion.\n"
            "Verifica que el servidor este corriendo y el audio sea valido.");
        return;
    }

    _repositorio.agregar(t);

    // Render con etiquetas "Persona 1/2/3..." en vez de A/B/C
    QString html = "<html><body style='font-family:Segoe UI,sans-serif;font-size:11pt'>";
    QString plano;
    for (const auto& seg : t.getSegmentos()) {
        QString speaker = QString::fromStdString(seg.getSpeaker());
        if (speaker.length() == 1 && speaker[0].isLetter()) {
            int num = speaker[0].toUpper().unicode() - 'A' + 1;
            speaker = QString("Persona %1").arg(num);
        }

        double ini = seg.getInicio(), fin = seg.getFin();
        QString txt = QString::fromStdString(seg.getTexto());

        // Formato hh:mm:ss para timestamps
        auto fmt = [](double s) {
            int total = static_cast<int>(s);
            int mm = total / 60, ss = total % 60;
            return QString("%1:%2").arg(mm, 2, 10, QLatin1Char('0'))
                                    .arg(ss, 2, 10, QLatin1Char('0'));
        };

        html += QString(
            "<p><span style='color:#555;font-size:9pt'>[%1 - %2]</span> "
            "<span style='color:#1565c0;font-weight:bold'>%3:</span> %4</p>")
            .arg(fmt(ini)).arg(fmt(fin)).arg(speaker).arg(txt);

        plano += QString("[%1 - %2] %3: %4\n")
            .arg(fmt(ini)).arg(fmt(fin)).arg(speaker).arg(txt);
    }
    html += "</body></html>";
    _areaOriginal->setHtml(html);
    _textoOriginal = plano;

    // Participacion
    const auto& part = t.getParticipacion();
    if (!part.empty()) {
        QStringList partes;
        for (const auto& [n, c] : part) {
            QString nom = QString::fromStdString(n);
            if (nom.length() == 1 && nom[0].isLetter()) {
                int num = nom[0].toUpper().unicode() - 'A' + 1;
                nom = QString("Persona %1").arg(num);
            }
            partes << QString("%1: %2s").arg(nom).arg(static_cast<int>(c));
        }
        _lblParticipacion->setText("Participacion: " + partes.join("  |  "));
    } else {
        _lblParticipacion->setText("");
    }

    _btnGuardar->setEnabled(true);
    _habilitarBotonesIA(true);
    _progress->setValue(100);
    statusBar()->showMessage(QString("Transcripcion lista — %1 segmentos")
                                 .arg(static_cast<int>(t.getSegmentos().size())));
}

void VentanaPrincipal::_habilitarBotonesIA(bool habilitado) {
    bool puede = habilitado && !_textoOriginal.isEmpty();
    _btnLimpiar  ->setEnabled(puede);
    _btnResumir  ->setEnabled(puede);
    _btnAnalizar ->setEnabled(puede);
    _btnTraducir ->setEnabled(puede);
    _btnPreguntar->setEnabled(puede);
}

// ════════════════════════════════════════════════════════════════════
//   IA: Limpiar / Resumir / Analizar
// ════════════════════════════════════════════════════════════════════
void VentanaPrincipal::_limpiar() {
    if (_textoOriginal.isEmpty()) return;
    _btnLimpiar->setEnabled(false);
    _areaLimpio->setPlainText("Procesando con IA...");
    statusBar()->showMessage("Limpiando con IA...");
    auto* cli = _transcriptor.get();
    std::string txt = _textoOriginal.toStdString();
    _watcherLimpiar->setFuture(QtConcurrent::run([cli, txt]{
        return cli->limpiar(txt);
    }));
}

void VentanaPrincipal::_resumir() {
    if (_textoOriginal.isEmpty()) return;
    _btnResumir->setEnabled(false);
    _areaResumen->setPlainText("Procesando con IA...");
    statusBar()->showMessage("Resumiendo con IA...");
    auto* cli = _transcriptor.get();
    std::string txt = _textoOriginal.toStdString();
    _watcherResumir->setFuture(QtConcurrent::run([cli, txt]{
        return cli->resumir(txt);
    }));
}

void VentanaPrincipal::_analizar() {
    if (_textoOriginal.isEmpty()) return;
    _btnAnalizar->setEnabled(false);
    _areaAnalisis->setPlainText("Procesando con IA...");
    statusBar()->showMessage("Analizando con IA...");
    auto* cli = _transcriptor.get();
    std::string txt = _textoOriginal.toStdString();
    _watcherAnalizar->setFuture(QtConcurrent::run([cli, txt]{
        return cli->analizar(txt);
    }));
}

void VentanaPrincipal::_onLimpiarListo() {
    auto r = _watcherLimpiar->result();
    _btnLimpiar->setEnabled(!_textoOriginal.isEmpty());
    if (r.ok) {
        _areaLimpio->setPlainText(QString::fromStdString(r.texto));
        statusBar()->showMessage("Limpieza lista.");
    } else {
        _areaLimpio->setPlainText("Error: " + QString::fromStdString(r.texto));
        statusBar()->showMessage("Error al limpiar.");
    }
}

void VentanaPrincipal::_onResumirListo() {
    auto r = _watcherResumir->result();
    _btnResumir->setEnabled(!_textoOriginal.isEmpty());
    if (r.ok) {
        _areaResumen->setPlainText(QString::fromStdString(r.texto));
        statusBar()->showMessage("Resumen listo.");
    } else {
        _areaResumen->setPlainText("Error: " + QString::fromStdString(r.texto));
        statusBar()->showMessage("Error al resumir.");
    }
}

void VentanaPrincipal::_onAnalizarListo() {
    auto r = _watcherAnalizar->result();
    _btnAnalizar->setEnabled(!_textoOriginal.isEmpty());
    if (r.ok) {
        _areaAnalisis->setPlainText(QString::fromStdString(r.texto));
        statusBar()->showMessage("Analisis listo.");
    } else {
        _areaAnalisis->setPlainText("Error: " + QString::fromStdString(r.texto));
        statusBar()->showMessage("Error al analizar.");
    }
}

// ════════════════════════════════════════════════════════════════════
//   IA: Traducir
// ════════════════════════════════════════════════════════════════════
void VentanaPrincipal::_traducir() {
    if (_textoOriginal.isEmpty()) return;
    _btnTraducir->setEnabled(false);

    QString nombreIdioma = _comboIdiomaTraducir->currentText();
    QString codigoIdioma = _comboIdiomaTraducir->currentData().toString();
    _areaTraducido->setPlainText(QString("Traduciendo al %1...").arg(nombreIdioma));
    statusBar()->showMessage(QString("Traduciendo al %1 con IA...").arg(nombreIdioma));

    auto* cli = _transcriptor.get();
    std::string txt    = _textoOriginal.toStdString();
    std::string idioma = nombreIdioma.toStdString();
    _watcherTraducir->setFuture(QtConcurrent::run([cli, txt, idioma]{
        return cli->traducir(txt, idioma);
    }));
}

void VentanaPrincipal::_onTraducirListo() {
    auto r = _watcherTraducir->result();
    _btnTraducir->setEnabled(!_textoOriginal.isEmpty());
    if (r.ok) {
        _areaTraducido->setPlainText(QString::fromStdString(r.texto));
        statusBar()->showMessage("Traduccion lista.");
    } else {
        _areaTraducido->setPlainText("Error: " + QString::fromStdString(r.texto));
        statusBar()->showMessage("Error al traducir.");
    }
}

// ════════════════════════════════════════════════════════════════════
//   IA: Chat Q&A
// ════════════════════════════════════════════════════════════════════
void VentanaPrincipal::_preguntar() {
    if (_textoOriginal.isEmpty()) {
        QMessageBox::information(this, "Sin contexto", "Primero transcribi un audio.");
        return;
    }
    QString preg = _inputPregunta->text().trimmed();
    if (preg.isEmpty()) return;

    _inputPregunta->clear();
    _areaChat->append(QString("\n<b>Vos:</b> %1").arg(preg.toHtmlEscaped()));
    _areaChat->append("<i>Pensando...</i>");
    _btnPreguntar->setEnabled(false);
    statusBar()->showMessage("Consultando a Gemini...");

    _preguntaPendiente = preg;
    auto* cli = _transcriptor.get();
    std::string txt  = _textoOriginal.toStdString();
    std::string pq   = preg.toStdString();
    auto historial   = _historialChat;
    _watcherPreguntar->setFuture(QtConcurrent::run([cli, txt, pq, historial]{
        return cli->preguntar(txt, pq, historial);
    }));
}

void VentanaPrincipal::_onPreguntarListo() {
    auto r = _watcherPreguntar->result();

    // Quitar el "Pensando..."
    QString contenido = _areaChat->toHtml();
    contenido.replace("<i>Pensando...</i>", "");
    _areaChat->setHtml(contenido);

    if (r.ok) {
        QString resp = QString::fromStdString(r.texto);
        _areaChat->append(QString("<b>IA:</b> %1<br>").arg(resp.toHtmlEscaped()));
        _historialChat.emplace_back(_preguntaPendiente.toStdString(), r.texto);
        statusBar()->showMessage("Listo.");
    } else {
        _areaChat->append(QString("<b>Error:</b> %1<br>")
                          .arg(QString::fromStdString(r.texto).toHtmlEscaped()));
        statusBar()->showMessage("Error en chat.");
    }
    _btnPreguntar->setEnabled(true);
}

// ════════════════════════════════════════════════════════════════════
//   Guardar todo
// ════════════════════════════════════════════════════════════════════
void VentanaPrincipal::_guardarTodo() {
    if (_textoOriginal.isEmpty()) return;

    QString base = QFileInfo(_rutaAudio).absolutePath() + "/"
                 + QFileInfo(_rutaAudio).completeBaseName();
    QStringList guardados;

    auto escribir = [&](const QString& sufijo, const QString& contenido) {
        if (contenido.trimmed().isEmpty()) return;
        QString p = base + "_" + sufijo + ".txt";
        QFile f(p);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&f);
            out.setEncoding(QStringConverter::Utf8);
            out << contenido;
            f.close();
            guardados << QFileInfo(p).fileName();
        }
    };

    escribir("transcripcion", _textoOriginal);
    escribir("limpio",        _areaLimpio   ->toPlainText());
    escribir("resumen",       _areaResumen  ->toPlainText());
    escribir("analisis",      _areaAnalisis ->toPlainText());
    escribir("traducido",     _areaTraducido->toPlainText());

    if (!guardados.isEmpty()) {
        QMessageBox::information(this, "Guardado",
            "Archivos generados:\n" + guardados.join("\n"));
    }
}

} // namespace DIArize::GUI
