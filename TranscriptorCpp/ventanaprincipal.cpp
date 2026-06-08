#include "ventanaprincipal.h"
#include "motortranscripcion.h"
#include "asistenteia.h"

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProcessEnvironment>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStatusBar>
#include <QStringConverter>
#include <QTabWidget>
#include <QTextEdit>
#include <QTextStream>
#include <QUrl>
#include <QVBoxLayout>

#include <QMediaCaptureSession>
#include <QAudioInput>
#include <QAudioDevice>
#include <QMediaRecorder>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QMediaDevices>

// Carpeta del motor Python por defecto (se puede cambiar desde el menu Configurar).
static const QString CARPETA_MOTOR_DEFAULT =
    "C:/Users/octav/OneDrive/Escritorio/UBP/3ro/POO/Proyecto POO/Transcriptor";

// (etiqueta visible, valor real) -- avisa velocidad/memoria de cada modelo.
struct ModeloInfo { const char *etiqueta; const char *valor; };
static const ModeloInfo MODELOS[] = {
    {"tiny (muy rapido, poco preciso)", "tiny"},
    {"base (rapido)",                   "base"},
    {"small (equilibrado)",             "small"},
    {"medium (recomendado)",            "medium"},
    {"large-v2 (lento, mucha memoria)", "large-v2"},
    {"large-v3 (lento, mucha memoria)", "large-v3"},
};

VentanaPrincipal::VentanaPrincipal(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Transcriptor de Audio con IA - Proyecto POO (C++)");
    resize(1100, 760);

    m_motor = new MotorTranscripcion(this);
    QSettings settings("UBP", "TranscriptorCpp");
    m_motor->setCarpetaMotor(settings.value("carpetaMotor", CARPETA_MOTOR_DEFAULT).toString());

    connect(m_motor, &MotorTranscripcion::progreso, this, &VentanaPrincipal::onProgreso);
    connect(m_motor, &MotorTranscripcion::terminado, this, &VentanaPrincipal::onTranscripcionLista);
    connect(m_motor, &MotorTranscripcion::error, this, &VentanaPrincipal::onErrorMotor);

    construirUi();
    aplicarEstilo();
    crearMenu();
    cargarIA();
}

// ------------------------------------------------------------------ UI
void VentanaPrincipal::construirUi()
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *layout = new QVBoxLayout(central);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    // --- Fila de configuracion ---
    QHBoxLayout *configBox = new QHBoxLayout();

    m_btnCargar = new QPushButton("Cargar audio");
    connect(m_btnCargar, &QPushButton::clicked, this, &VentanaPrincipal::cargarAudio);
    configBox->addWidget(m_btnCargar);

    m_btnGrabar = new QPushButton(QString::fromUtf8("● Grabar"));
    m_btnGrabar->setToolTip("Grabar audio desde el microfono y usarlo para transcribir");
    connect(m_btnGrabar, &QPushButton::clicked, this, &VentanaPrincipal::toggleGrabacion);
    configBox->addWidget(m_btnGrabar);

    m_btnReproducir = new QPushButton(QString::fromUtf8("▶ Reproducir"));
    m_btnReproducir->setToolTip("Escuchar el audio cargado o grabado");
    m_btnReproducir->setEnabled(false);
    connect(m_btnReproducir, &QPushButton::clicked, this, &VentanaPrincipal::toggleReproduccion);
    configBox->addWidget(m_btnReproducir);

    m_lblArchivo = new QLabel("Ningun archivo seleccionado");
    m_lblArchivo->setStyleSheet("color: #888;");
    configBox->addWidget(m_lblArchivo, 1);

    configBox->addWidget(new QLabel("Modelo:"));
    m_comboModelo = new QComboBox();
    for (const auto &m : MODELOS)
        m_comboModelo->addItem(m.etiqueta, m.valor);
    m_comboModelo->setCurrentIndex(3);  // medium
    configBox->addWidget(m_comboModelo);

    configBox->addWidget(new QLabel("Idioma:"));
    m_comboIdioma = new QComboBox();
    m_comboIdioma->addItems({"es", "en", "pt", "fr", "it", "de"});
    configBox->addWidget(m_comboIdioma);

    configBox->addWidget(new QLabel("Cantidad de personas:"));
    m_spinPersonas = new QSpinBox();
    m_spinPersonas->setRange(1, 10);
    m_spinPersonas->setValue(2);
    m_spinPersonas->setToolTip("Indica cuantas personas hablan. El numero exacto da la mejor diarizacion.");
    configBox->addWidget(m_spinPersonas);

    layout->addLayout(configBox);

    // --- Boton transcribir + progreso ---
    QHBoxLayout *accionBox = new QHBoxLayout();
    m_btnTranscribir = new QPushButton("Transcribir audio");
    m_btnTranscribir->setMinimumHeight(36);
    m_btnTranscribir->setEnabled(false);
    connect(m_btnTranscribir, &QPushButton::clicked, this, &VentanaPrincipal::transcribir);
    accionBox->addWidget(m_btnTranscribir);

    m_progress = new QProgressBar();
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    accionBox->addWidget(m_progress, 1);
    layout->addLayout(accionBox);

    // --- Tabs ---
    m_tabs = new QTabWidget();
    layout->addWidget(m_tabs, 1);

    auto nuevaArea = [](const QString &ph) {
        QTextEdit *t = new QTextEdit();
        t->setPlaceholderText(ph);
        t->setFont(QFont("Consolas", 10));
        return t;
    };
    auto tabConBoton = [&](const QString &etiquetaBoton, QTextEdit **area, QPushButton **boton) {
        QWidget *w = new QWidget();
        QVBoxLayout *l = new QVBoxLayout(w);
        l->setContentsMargins(0, 0, 0, 0);
        *boton = new QPushButton(etiquetaBoton);
        (*boton)->setEnabled(false);
        *area = nuevaArea("Resultado de '" + etiquetaBoton + "'...");
        l->addWidget(*boton);
        l->addWidget(*area, 1);
        return w;
    };

    // Tab 1: original
    m_txtOriginal = nuevaArea("Aca aparece la transcripcion original con timestamps y personas...");
    m_tabs->addTab(m_txtOriginal, "Original");

    // Tab 2-4: limpio / resumen / analisis
    m_tabs->addTab(tabConBoton("Limpiar con IA", &m_txtLimpio, &m_btnLimpiar), "Limpio");
    connect(m_btnLimpiar, &QPushButton::clicked, this, &VentanaPrincipal::limpiarTexto);
    m_tabs->addTab(tabConBoton("Resumir con IA", &m_txtResumen, &m_btnResumir), "Resumen");
    connect(m_btnResumir, &QPushButton::clicked, this, &VentanaPrincipal::resumir);
    m_tabs->addTab(tabConBoton("Analizar con IA", &m_txtAnalisis, &m_btnAnalizar), "Analisis");
    connect(m_btnAnalizar, &QPushButton::clicked, this, &VentanaPrincipal::analizar);

    // Tab 5: Chat Q&A
    QWidget *chatWidget = new QWidget();
    QVBoxLayout *chatLayout = new QVBoxLayout(chatWidget);
    chatLayout->setContentsMargins(0, 0, 0, 0);
    m_txtChat = new QTextEdit();
    m_txtChat->setReadOnly(true);
    m_txtChat->setPlaceholderText("Hacele preguntas sobre lo que se dijo en el audio.");
    chatLayout->addWidget(m_txtChat, 1);
    QHBoxLayout *chatInput = new QHBoxLayout();
    m_inputPregunta = new QLineEdit();
    m_inputPregunta->setPlaceholderText("Escribi tu pregunta y enter...");
    connect(m_inputPregunta, &QLineEdit::returnPressed, this, &VentanaPrincipal::preguntar);
    chatInput->addWidget(m_inputPregunta, 1);
    m_btnPreguntar = new QPushButton("Preguntar");
    connect(m_btnPreguntar, &QPushButton::clicked, this, &VentanaPrincipal::preguntar);
    chatInput->addWidget(m_btnPreguntar);
    chatLayout->addLayout(chatInput);
    m_tabs->addTab(chatWidget, "Chat (Q&A)");

    // --- Fila inferior ---
    QHBoxLayout *bottomBox = new QHBoxLayout();
    m_lblParticipacion = new QLabel("");
    m_lblParticipacion->setStyleSheet("color: #555;");
    bottomBox->addWidget(m_lblParticipacion, 1);
    m_btnGuardar = new QPushButton("Guardar todo (.txt)");
    m_btnGuardar->setEnabled(false);
    connect(m_btnGuardar, &QPushButton::clicked, this, &VentanaPrincipal::guardarTodo);
    bottomBox->addWidget(m_btnGuardar);
    layout->addLayout(bottomBox);

    setStatusBar(new QStatusBar(this));
    setEstado("Listo. Carga un archivo de audio para empezar.");
}

void VentanaPrincipal::aplicarEstilo()
{
    setStyleSheet(R"(
        QPushButton {
            background-color: #2563eb; color: white; border-radius: 6px;
            padding: 6px 14px; font-weight: 600;
        }
        QPushButton:disabled { background-color: #94a3b8; }
        QPushButton:hover:!disabled { background-color: #1d4ed8; }
        QTabWidget::pane { border: 1px solid #cbd5e1; }
        QTextEdit { border: 1px solid #cbd5e1; padding: 6px; }
        QLineEdit { border: 1px solid #cbd5e1; padding: 4px 6px; border-radius: 4px; }
        QProgressBar { border: 1px solid #cbd5e1; border-radius: 4px; text-align: center; }
        QProgressBar::chunk { background-color: #2563eb; }
    )");
}

void VentanaPrincipal::crearMenu()
{
    QMenu *menu = menuBar()->addMenu("Configurar");
    QAction *act = menu->addAction("Carpeta del motor Python...");
    connect(act, &QAction::triggered, this, &VentanaPrincipal::configurarCarpetaMotor);
}

// ------------------------------------------------------------------ IA
QString VentanaPrincipal::leerEnv(const QString &clave, const QString &carpetaMotor)
{
    // 1) Variable de entorno
    const QString env = QProcessEnvironment::systemEnvironment().value(clave);
    if (!env.isEmpty())
        return env;

    // 2) Archivo .env de la carpeta del motor (lineas no comentadas)
    QFile f(QDir(carpetaMotor).filePath(".env"));
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&f);
        while (!in.atEnd()) {
            const QString linea = in.readLine().trimmed();
            if (linea.startsWith('#'))
                continue;
            const int eq = linea.indexOf('=');
            if (eq > 0 && linea.left(eq).trimmed() == clave)
                return linea.mid(eq + 1).trimmed();
        }
    }
    return QString();
}

void VentanaPrincipal::cargarIA()
{
    const QString carpeta = m_motor->carpetaMotor();
    const QString key = leerEnv("OPENAI_API_KEY", carpeta);
    if (key.isEmpty()) {
        setEstado("Sin API key de IA -- funciones de IA deshabilitadas.");
        return;
    }

    QString modelo = leerEnv("OPENAI_MODEL", carpeta);
    if (modelo.isEmpty())
        modelo = "gemini-2.5-flash";

    // OPENAI_BASE_URL viene en formato SDK (.../openai/); armamos el endpoint completo.
    QString base = leerEnv("OPENAI_BASE_URL", carpeta);
    if (base.isEmpty())
        base = "https://generativelanguage.googleapis.com/v1beta/openai/";
    if (!base.endsWith('/'))
        base += '/';
    const QString endpoint = base + "chat/completions";

    m_ia = new AsistenteIA(key, modelo, endpoint, this);
    connect(m_ia, &AsistenteIA::respuestaLista, this, &VentanaPrincipal::onRespuestaIA);
    connect(m_ia, &AsistenteIA::errorIA, this, &VentanaPrincipal::onErrorIA);
}

// ------------------------------------------------------------------ audio: cargar
void VentanaPrincipal::cargarAudio()
{
    const QString ruta = QFileDialog::getOpenFileName(
        this, "Seleccionar archivo de audio", QDir::homePath(),
        "Audio (*.mp3 *.wav *.m4a *.flac *.ogg *.aac);;Todos los archivos (*.*)");
    if (ruta.isEmpty())
        return;
    detenerReproduccion();
    m_rutaAudio = ruta;
    m_lblArchivo->setText(QFileInfo(ruta).fileName());
    m_lblArchivo->setStyleSheet("color: #111;");
    m_btnTranscribir->setEnabled(true);
    m_btnReproducir->setEnabled(true);
    setEstado("Archivo cargado: " + QFileInfo(ruta).fileName());
}

// ------------------------------------------------------------------ audio: grabar
void VentanaPrincipal::initGrabador()
{
    if (m_grabador)
        return;
    if (QMediaDevices::audioInputs().isEmpty()) {
        QMessageBox::warning(this, "Sin microfono", "No se detecto ningun microfono.");
        return;
    }
    m_captura = new QMediaCaptureSession(this);
    m_audioInput = new QAudioInput(this);
    m_captura->setAudioInput(m_audioInput);
    m_grabador = new QMediaRecorder(this);
    m_captura->setRecorder(m_grabador);
    connect(m_grabador, &QMediaRecorder::recorderStateChanged,
            this, &VentanaPrincipal::onGrabadorEstado);
    connect(m_grabador, &QMediaRecorder::errorOccurred, this,
            [this](QMediaRecorder::Error, const QString &cad) {
                QMessageBox::critical(this, "Error de grabacion", cad);
            });
}

bool VentanaPrincipal::grabando() const
{
    return m_grabador && m_grabador->recorderState() == QMediaRecorder::RecordingState;
}

void VentanaPrincipal::toggleGrabacion()
{
    initGrabador();
    if (!m_grabador)
        return;
    if (grabando()) {
        m_grabador->stop();
    } else {
        detenerReproduccion();
        const QString destino = QDir(QDir::tempPath()).filePath("grabacion_transcriptor");
        m_grabador->setOutputLocation(QUrl::fromLocalFile(destino));
        m_grabador->record();
    }
}

void VentanaPrincipal::onGrabadorEstado()
{
    if (grabando()) {
        m_btnGrabar->setText(QString::fromUtf8("■ Detener"));
        m_btnCargar->setEnabled(false);
        m_btnTranscribir->setEnabled(false);
        setEstado("Grabando... apreta Detener cuando termines.");
    } else if (m_grabador && m_grabador->recorderState() == QMediaRecorder::StoppedState) {
        m_btnGrabar->setText(QString::fromUtf8("● Grabar"));
        m_btnCargar->setEnabled(true);
        const QString ruta = m_grabador->actualLocation().toLocalFile();
        if (!ruta.isEmpty() && QFileInfo::exists(ruta)) {
            m_rutaAudio = ruta;
            m_lblArchivo->setText(QFileInfo(ruta).fileName() + " (grabado)");
            m_lblArchivo->setStyleSheet("color: #111;");
            m_btnTranscribir->setEnabled(true);
            m_btnReproducir->setEnabled(true);
            setEstado("Grabacion lista: " + QFileInfo(ruta).fileName());
        } else {
            setEstado("No se pudo guardar la grabacion.");
        }
    }
}

// ------------------------------------------------------------------ audio: reproducir
void VentanaPrincipal::initPlayer()
{
    if (m_player)
        return;
    m_audioOutput = new QAudioOutput(this);
    m_player = new QMediaPlayer(this);
    m_player->setAudioOutput(m_audioOutput);
    connect(m_player, &QMediaPlayer::playbackStateChanged,
            this, &VentanaPrincipal::onPlayerEstado);
}

void VentanaPrincipal::toggleReproduccion()
{
    if (m_rutaAudio.isEmpty())
        return;
    initPlayer();
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_player->stop();
    } else {
        m_player->setSource(QUrl::fromLocalFile(m_rutaAudio));
        m_player->play();
    }
}

void VentanaPrincipal::onPlayerEstado()
{
    if (m_player && m_player->playbackState() == QMediaPlayer::PlayingState)
        m_btnReproducir->setText(QString::fromUtf8("■ Detener"));
    else
        m_btnReproducir->setText(QString::fromUtf8("▶ Reproducir"));
}

void VentanaPrincipal::detenerReproduccion()
{
    if (m_player)
        m_player->stop();
}

// ------------------------------------------------------------------ transcripcion
void VentanaPrincipal::transcribir()
{
    if (m_rutaAudio.isEmpty())
        return;
    if (grabando()) {
        QMessageBox::information(this, "Grabando", "Detene la grabacion antes de transcribir.");
        return;
    }

    desactivarBotones(true);
    m_progress->setValue(0);
    setEstado("Procesando audio... (puede tardar varios minutos)");

    m_motor->transcribir(m_rutaAudio, m_spinPersonas->value(),
                         m_comboModelo->currentData().toString(),
                         m_comboIdioma->currentText());
}

void VentanaPrincipal::onProgreso(const QString &etapa, int pct)
{
    if (pct >= 0)
        m_progress->setValue(pct);
    setEstado(etapa);
}

void VentanaPrincipal::onTranscripcionLista(const QString &texto, const QString &participacion)
{
    m_textoOriginal = texto;
    m_txtOriginal->setPlainText(texto);
    m_lblParticipacion->setText(participacion.isEmpty() ? QString()
                                                        : "Participacion: " + participacion);

    desactivarBotones(false);
    m_btnGuardar->setEnabled(true);
    if (m_ia) {
        m_btnLimpiar->setEnabled(true);
        m_btnResumir->setEnabled(true);
        m_btnAnalizar->setEnabled(true);
    }
    m_progress->setValue(100);
    setEstado("Transcripcion lista");
}

void VentanaPrincipal::onErrorMotor(const QString &mensaje)
{
    desactivarBotones(false);
    m_progress->setValue(0);
    setEstado("Error en el procesamiento");
    QMessageBox::critical(this, "Error", mensaje);
}

void VentanaPrincipal::desactivarBotones(bool procesando)
{
    m_btnTranscribir->setEnabled(!procesando && !m_rutaAudio.isEmpty());
    m_btnCargar->setEnabled(!procesando);
    m_btnGrabar->setEnabled(!procesando);
    m_btnReproducir->setEnabled(!procesando && !m_rutaAudio.isEmpty());
    if (procesando) {
        detenerReproduccion();
        m_btnLimpiar->setEnabled(false);
        m_btnResumir->setEnabled(false);
        m_btnAnalizar->setEnabled(false);
        m_btnGuardar->setEnabled(false);
    }
}

// ------------------------------------------------------------------ IA acciones
void VentanaPrincipal::limpiarTexto()
{
    if (!m_ia || m_textoOriginal.isEmpty()) return;
    m_txtLimpio->setPlainText("Procesando 'Limpieza'...");
    setEstado("Llamando a OpenAI para 'Limpieza'...");
    m_ia->limpiar(m_textoOriginal);
}

void VentanaPrincipal::resumir()
{
    if (!m_ia || m_textoOriginal.isEmpty()) return;
    m_txtResumen->setPlainText("Procesando 'Resumen'...");
    setEstado("Llamando a OpenAI para 'Resumen'...");
    m_ia->resumir(m_textoOriginal);
}

void VentanaPrincipal::analizar()
{
    if (!m_ia || m_textoOriginal.isEmpty()) return;
    m_txtAnalisis->setPlainText("Procesando 'Analisis'...");
    setEstado("Llamando a OpenAI para 'Analisis'...");
    m_ia->analizar(m_textoOriginal);
}

void VentanaPrincipal::preguntar()
{
    if (!m_ia || m_textoOriginal.isEmpty()) {
        QMessageBox::information(this, "Sin contexto", "Primero transcribi un audio.");
        return;
    }
    const QString pregunta = m_inputPregunta->text().trimmed();
    if (pregunta.isEmpty())
        return;
    m_inputPregunta->clear();
    m_preguntaPendiente = pregunta;
    m_txtChat->append("<b>Vos:</b> " + pregunta.toHtmlEscaped());
    setEstado("Consultando a OpenAI...");
    m_btnPreguntar->setEnabled(false);
    m_ia->preguntar(m_textoOriginal, pregunta, m_historialChat);
}

void VentanaPrincipal::onRespuestaIA(const QString &tipo, const QString &contenido)
{
    if (tipo == "limpieza") {
        m_txtLimpio->setPlainText(contenido);
        setEstado("Limpieza lista");
    } else if (tipo == "resumen") {
        m_txtResumen->setPlainText(contenido);
        setEstado("Resumen listo");
    } else if (tipo == "analisis") {
        m_txtAnalisis->setPlainText(contenido);
        setEstado("Analisis listo");
    } else if (tipo == "qa") {
        m_txtChat->append("<b>IA:</b> " + contenido.toHtmlEscaped() + "<br>");
        m_historialChat.append(qMakePair(m_preguntaPendiente, contenido));
        m_btnPreguntar->setEnabled(true);
        setEstado("Listo");
    }
}

void VentanaPrincipal::onErrorIA(const QString &tipo, const QString &mensaje)
{
    if (tipo == "qa")
        m_btnPreguntar->setEnabled(true);
    setEstado("Error de IA");
    QMessageBox::critical(this, "Error de IA", mensaje);
}

// ------------------------------------------------------------------ guardar / config
void VentanaPrincipal::guardarTodo()
{
    if (m_rutaAudio.isEmpty())
        return;
    QFileInfo info(m_rutaAudio);
    const QString base = info.dir().filePath(info.completeBaseName());
    QStringList guardados;

    auto guardar = [&](const QString &sufijo, const QString &contenido) {
        if (contenido.trimmed().isEmpty())
            return;
        const QString ruta = base + sufijo;
        QFile f(ruta);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&f);
            out.setEncoding(QStringConverter::Utf8);
            out << contenido;
            guardados << QFileInfo(ruta).fileName();
        }
    };

    guardar("_transcripcion.txt", m_textoOriginal);
    guardar("_limpio.txt", m_txtLimpio->toPlainText());
    guardar("_resumen.txt", m_txtResumen->toPlainText());
    guardar("_analisis.txt", m_txtAnalisis->toPlainText());

    if (!guardados.isEmpty())
        QMessageBox::information(this, "Guardado",
                                 "Archivos generados:\n" + guardados.join('\n'));
}

void VentanaPrincipal::configurarCarpetaMotor()
{
    const QString carpeta = QFileDialog::getExistingDirectory(
        this, "Elegi la carpeta del motor Python (contiene cli.py y venv)",
        m_motor->carpetaMotor());
    if (carpeta.isEmpty())
        return;
    m_motor->setCarpetaMotor(carpeta);
    QSettings("UBP", "TranscriptorCpp").setValue("carpetaMotor", carpeta);

    QString motivo;
    if (m_motor->configuracionValida(&motivo))
        setEstado("Motor configurado: " + carpeta);
    else
        QMessageBox::warning(this, "Configuracion incompleta", motivo);
}

void VentanaPrincipal::setEstado(const QString &msg)
{
    statusBar()->showMessage(msg);
}
