#ifndef VENTANAPRINCIPAL_H
#define VENTANAPRINCIPAL_H

#include <QMainWindow>
#include <QString>
#include <QVector>
#include <QPair>

class QComboBox;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QSpinBox;
class QTabWidget;
class QTextEdit;

class QMediaCaptureSession;
class QAudioInput;
class QMediaRecorder;
class QMediaPlayer;
class QAudioOutput;

class MotorTranscripcion;
class AsistenteIA;

/**
 * @brief Ventana principal de la app (equivalente C++ de la GUI Python).
 *
 * Coordina la transcripcion (MotorTranscripcion -> motor Python) y el
 * procesamiento con IA (AsistenteIA -> OpenAI), mas grabar/reproducir audio.
 */
class VentanaPrincipal : public QMainWindow
{
    Q_OBJECT

public:
    explicit VentanaPrincipal(QWidget *parent = nullptr);

private slots:
    // Audio
    void cargarAudio();
    void toggleGrabacion();
    void onGrabadorEstado();
    void toggleReproduccion();
    void onPlayerEstado();

    // Transcripcion
    void transcribir();
    void onProgreso(const QString &etapa, int pct);
    void onTranscripcionLista(const QString &texto, const QString &participacion);
    void onErrorMotor(const QString &mensaje);

    // IA
    void limpiarTexto();
    void resumir();
    void analizar();
    void preguntar();
    void onRespuestaIA(const QString &tipo, const QString &contenido);
    void onErrorIA(const QString &tipo, const QString &mensaje);

    // Guardar / config
    void guardarTodo();
    void configurarCarpetaMotor();

private:
    void construirUi();
    void aplicarEstilo();
    void crearMenu();
    void cargarIA();
    void desactivarBotones(bool procesando);
    void detenerReproduccion();
    bool grabando() const;
    void initGrabador();
    void initPlayer();
    void setEstado(const QString &msg);
    static QString leerEnv(const QString &clave, const QString &carpetaMotor);

    // --- Estado ---
    QString m_rutaAudio;
    QString m_textoOriginal;
    QString m_preguntaPendiente;
    QVector<QPair<QString, QString>> m_historialChat;

    MotorTranscripcion *m_motor = nullptr;
    AsistenteIA *m_ia = nullptr;

    // --- Audio (grabacion / reproduccion) ---
    QMediaCaptureSession *m_captura = nullptr;
    QAudioInput *m_audioInput = nullptr;
    QMediaRecorder *m_grabador = nullptr;
    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audioOutput = nullptr;

    // --- Widgets ---
    QPushButton *m_btnCargar = nullptr;
    QPushButton *m_btnGrabar = nullptr;
    QPushButton *m_btnReproducir = nullptr;
    QLabel *m_lblArchivo = nullptr;
    QComboBox *m_comboModelo = nullptr;
    QComboBox *m_comboIdioma = nullptr;
    QSpinBox *m_spinPersonas = nullptr;
    QPushButton *m_btnTranscribir = nullptr;
    QProgressBar *m_progress = nullptr;
    QTabWidget *m_tabs = nullptr;
    QTextEdit *m_txtOriginal = nullptr;
    QTextEdit *m_txtLimpio = nullptr;
    QPushButton *m_btnLimpiar = nullptr;
    QTextEdit *m_txtResumen = nullptr;
    QPushButton *m_btnResumir = nullptr;
    QTextEdit *m_txtAnalisis = nullptr;
    QPushButton *m_btnAnalizar = nullptr;
    QTextEdit *m_txtChat = nullptr;
    QLineEdit *m_inputPregunta = nullptr;
    QPushButton *m_btnPreguntar = nullptr;
    QLabel *m_lblParticipacion = nullptr;
    QPushButton *m_btnGuardar = nullptr;
};

#endif // VENTANAPRINCIPAL_H
