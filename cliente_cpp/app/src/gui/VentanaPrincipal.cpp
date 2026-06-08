#include "gui/VentanaPrincipal.h"
#include "gui/PanelHistorial.h"
#include "gui/PanelTranscripcion.h"
#include "red/ClienteTranscriptor.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QStatusBar>
#include <QVBoxLayout>

namespace DIArize::GUI {

VentanaPrincipal::VentanaPrincipal(QWidget* parent)
    : QMainWindow(parent)
    , _transcriptor(std::make_unique<DIArize::Red::ClienteTranscriptor>())
    , _lblArchivo(new QLabel("Ningun archivo seleccionado", this))
    , _btnSelec(new QPushButton("Seleccionar audio...", this))
    , _progress(new QProgressBar(this))
    , _btnTranscribir(new QPushButton("Transcribir", this))
    , _tabs(new QTabWidget(this))
{
    setWindowTitle("DIArize C++");
    resize(960, 680);

    // ── Paneles (polimorfismo via PanelBase*) ─────────────────────
    auto* panelT = new PanelTranscripcion(this);
    auto* panelH = new PanelHistorial(this);
    _paneles = { panelT, panelH };
    _tabs->addTab(panelT, panelT->titulo());
    _tabs->addTab(panelH, panelH->titulo());

    // ── Toolbar ───────────────────────────────────────────────────
    auto* toolbar   = new QFrame(this);
    auto* lblTitulo = new QLabel("DIArize", toolbar);
    toolbar->setObjectName("toolbar");
    _btnSelec->setObjectName("btnSelec");
    _lblArchivo->setObjectName("lblArchivo");
    lblTitulo->setObjectName("lblTitulo");

    _btnSelec->setEnabled(false);          // se habilitan cuando el server este listo
    _btnTranscribir->setEnabled(false);

    auto* toolLay = new QHBoxLayout(toolbar);
    toolLay->setContentsMargins(12, 6, 12, 6);
    toolLay->setSpacing(10);
    toolLay->addWidget(lblTitulo);
    toolLay->addSpacing(16);
    toolLay->addWidget(_lblArchivo, 1);
    toolLay->addWidget(_btnSelec);
    toolLay->addWidget(_btnTranscribir);

    // ── Layout central ────────────────────────────────────────────
    auto* central = new QWidget(this);
    central->setStyleSheet("background-color: #FFFFFF;");
    auto* vlay = new QVBoxLayout(central);
    vlay->setContentsMargins(0, 0, 0, 0);
    vlay->setSpacing(0);
    vlay->addWidget(toolbar);
    vlay->addWidget(_tabs, 1);
    setCentralWidget(central);

    // ── Status bar ────────────────────────────────────────────────
    _progress->setRange(0, 0);           // indeterminate mientras arranca
    _progress->setTextVisible(false);
    _progress->setFixedWidth(180);
    statusBar()->addPermanentWidget(_progress);
    statusBar()->showMessage("Iniciando servidor...");

    // ── Signals & slots ───────────────────────────────────────────
    connect(_btnSelec,       &QPushButton::clicked, this, &VentanaPrincipal::_seleccionarArchivo);
    connect(_btnTranscribir, &QPushButton::clicked, this, &VentanaPrincipal::_transcribir);

    // Arranca el servidor Flask automaticamente
    _iniciarServidor();
}

VentanaPrincipal::~VentanaPrincipal() = default;

// ── Inicio automatico del servidor ───────────────────────────────

void VentanaPrincipal::_iniciarServidor() {
    QString appDir = QCoreApplication::applicationDirPath();

    // Busca servidor.exe en el mismo directorio que el ejecutable
    QString exePath = appDir + "/servidor.exe";

    _serverProcess = new QProcess(this);

    if (QFile::exists(exePath)) {
        // Produccion: servidor.exe generado con PyInstaller
        _serverProcess->setWorkingDirectory(appDir);
        _serverProcess->start(exePath, {});
    } else {
        // Desarrollo: python + ruta al servidor.py
        // Busca el venv en ubicaciones comunes del proyecto
        QStringList pythonCandidates = {
            appDir + "/../../../../../Facu/Tercer Año/POO/"
                "DIArize_POO_Brachetti_Diaz_Lopardo/venv/Scripts/pythonw.exe",
            appDir + "/../../../../venv/Scripts/pythonw.exe",
            "pythonw"
        };
        QStringList serverCandidates = {
            appDir + "/../../../../../Facu/Tercer Año/POO/"
                "DIArize_POO_Brachetti_Diaz_Lopardo/Transcriptor/servidor.py",
            appDir + "/../../../../Transcriptor/servidor.py",
        };

        QString python, server;
        for (const auto& p : pythonCandidates) {
            if (QFile::exists(p)) { python = p; break; }
        }
        for (const auto& s : serverCandidates) {
            if (QFile::exists(s)) { server = s; break; }
        }

        if (!python.isEmpty() && !server.isEmpty()) {
            _serverProcess->setWorkingDirectory(QFileInfo(server).absolutePath());
            _serverProcess->start(python, { server });
        } else {
            // Servidor ya estaba corriendo externamente — solo verificamos
        }
    }

    // Timer: revisa cada segundo si el servidor ya responde
    _startupTimer = new QTimer(this);
    connect(_startupTimer, &QTimer::timeout,
            this, &VentanaPrincipal::_verificarServidor);
    _startupTimer->start(1000);
}

void VentanaPrincipal::_verificarServidor() {
    _startupAttempts++;

    if (_transcriptor->estaDisponible()) {
        _startupTimer->stop();
        _progress->setRange(0, 100);
        _progress->setValue(0);
        _btnSelec->setEnabled(true);
        statusBar()->showMessage("Listo.  Selecciona un archivo de audio para comenzar.");
        return;
    }

    if (_startupAttempts >= 30) {
        _startupTimer->stop();
        _progress->setRange(0, 100);
        _progress->setValue(0);
        statusBar()->showMessage("Error: no se pudo conectar al servidor.");
        QMessageBox::critical(this, "Error de inicio",
            "El servidor no respondio despues de 30 segundos.\n\n"
            "Asegurate de que servidor.exe este en la misma carpeta que DIArizeCpp.exe.");
        return;
    }

    statusBar()->showMessage(
        QString("Iniciando servidor...  %1s").arg(_startupAttempts));
}

// ── closeEvent: apaga el servidor al cerrar ───────────────────────

void VentanaPrincipal::closeEvent(QCloseEvent* e) {
    if (_startupTimer)  _startupTimer->stop();
    if (_serverProcess && _serverProcess->state() != QProcess::NotRunning) {
        _serverProcess->terminate();
        _serverProcess->waitForFinished(3000);
        if (_serverProcess->state() != QProcess::NotRunning)
            _serverProcess->kill();
    }
    QMainWindow::closeEvent(e);
}

// ── IObservador ───────────────────────────────────────────────────

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

// ── Slots ─────────────────────────────────────────────────────────

void VentanaPrincipal::_seleccionarArchivo() {
    QString ruta = QFileDialog::getOpenFileName(
        this, "Seleccionar audio", QString(),
        "Audio (*.mp3 *.wav *.m4a *.flac *.ogg);;Todos (*.*)");
    if (ruta.isEmpty()) return;
    _rutaAudio = ruta;
    _lblArchivo->setText(QFileInfo(ruta).fileName());
    _btnTranscribir->setEnabled(true);
}

void VentanaPrincipal::_transcribir() {
    if (_rutaAudio.isEmpty()) return;
    _btnTranscribir->setEnabled(false);
    onEstadoCambiado(DIArize::Core::EstadoApp::Procesando, "Enviando audio al servidor...");

    DIArize::Core::Transcripcion t = _transcriptor->transcribir(_rutaAudio.toStdString());

    if (t.getSegmentos().empty() && t.getTexto().empty()) {
        onEstadoCambiado(DIArize::Core::EstadoApp::Error,
                         "Error: no se obtuvo respuesta del servidor.");
        QMessageBox::warning(this, "Error",
            "No se pudo obtener una transcripcion.\n"
            "Verifica que el servidor este corriendo y el audio sea valido.");
    } else {
        _repositorio.agregar(t);
        _actualizarPaneles(t);
        onEstadoCambiado(DIArize::Core::EstadoApp::Listo,
                         QString("Listo — %1 segmentos")
                             .arg(static_cast<int>(t.getSegmentos().size()))
                             .toStdString());
        _progress->setValue(100);
    }
    _btnTranscribir->setEnabled(true);
}

void VentanaPrincipal::_actualizarPaneles(const DIArize::Core::Transcripcion& t) {
    for (PanelBase* panel : _paneles)
        panel->actualizar(t);
}

} // namespace DIArize::GUI
