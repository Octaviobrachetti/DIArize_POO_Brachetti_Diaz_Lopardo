#pragma once
#include <QAudioInput>
#include <QAudioOutput>
#include <QCloseEvent>
#include <QComboBox>
#include <QFutureWatcher>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMediaCaptureSession>
#include <QMediaPlayer>
#include <QMediaRecorder>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTextEdit>
#include <QTimer>
#include <memory>
#include <utility>
#include <vector>

#include "gui/PanelEnVivo.h"
#include "nucleo/IObservador.h"
#include "nucleo/Repositorio.h"
#include "red/ClienteTranscriptor.h"

namespace DIArize::GUI {

class VentanaPrincipal : public QMainWindow,
                          public DIArize::Core::IObservador {
    Q_OBJECT

public:
    explicit VentanaPrincipal(QWidget* parent = nullptr);
    ~VentanaPrincipal() override;

    void onEstadoCambiado(DIArize::Core::EstadoApp estado,
                          const std::string& detalle) override;
    void onProgreso(int porcentaje, const std::string& etapa) override;

protected:
    void closeEvent(QCloseEvent* e) override;

private slots:
    // archivo
    void _seleccionarArchivo();
    void _quitarAudio();

    // grabacion
    void _toggleGrabacion();
    void _tickGrabacion();

    // reproduccion
    void _toggleReproduccion();

    // transcripcion
    void _transcribir();
    void _onTranscripcionLista();

    // IA
    void _limpiar();
    void _resumir();
    void _analizar();
    void _traducir();
    void _preguntar();
    void _onLimpiarListo();
    void _onResumirListo();
    void _onAnalizarListo();
    void _onTraducirListo();
    void _onPreguntarListo();

    // otros
    void _guardarTodo();
    void _verificarServidor();

private:
    void _iniciarServidor();
    void _construirUI();
    QWidget* _construirModoArchivo();
    QWidget* _construirToggleModo();
    QWidget* _construirTabIA(QTextEdit*& areaSalida,
                              QPushButton*& boton,
                              const QString& etiquetaBoton,
                              const QString& placeholder);
    QWidget* _construirTabChat();

private slots:
    void _cambiarAModoArchivo();
    void _cambiarAModoEnVivo();
    void _procesarWavDeEnVivo(const QString& rutaWav);

private:
    void _setRutaAudio(const QString& ruta, const QString& displayName);
    void _habilitarBotonesIA(bool habilitado);
    QString _formatoOriginal() const;

    // Backend
    std::unique_ptr<DIArize::Red::ClienteTranscriptor> _transcriptor;
    DIArize::Core::Repositorio                         _repositorio;
    QString                                            _rutaAudio;
    QString                                            _textoOriginal;
    std::vector<std::pair<std::string,std::string>>    _historialChat;

    // Proceso del servidor Flask (arrancado automaticamente)
    QProcess* _serverProcess   = nullptr;
    QTimer*   _startupTimer    = nullptr;
    int       _startupAttempts = 0;

    // Grabacion / Reproduccion
    QMediaCaptureSession* _captura      = nullptr;
    QAudioInput*          _audioInput   = nullptr;
    QMediaRecorder*       _grabador     = nullptr;
    QMediaPlayer*         _player       = nullptr;
    QAudioOutput*         _audioOutput  = nullptr;
    QTimer*               _timerRec     = nullptr;
    int                   _segGrabando  = 0;

    // Workers asincronicos (HTTP en background)
    QFutureWatcher<DIArize::Core::Transcripcion>* _watcherTranscribir = nullptr;
    QFutureWatcher<DIArize::Red::RespuestaIA>*    _watcherLimpiar     = nullptr;
    QFutureWatcher<DIArize::Red::RespuestaIA>*    _watcherResumir     = nullptr;
    QFutureWatcher<DIArize::Red::RespuestaIA>*    _watcherAnalizar    = nullptr;
    QFutureWatcher<DIArize::Red::RespuestaIA>*    _watcherTraducir    = nullptr;
    QFutureWatcher<DIArize::Red::RespuestaIA>*    _watcherPreguntar   = nullptr;
    QString                                       _preguntaPendiente;

    // Widgets - toolbar
    QPushButton*  _btnCargar;
    QPushButton*  _btnGrabar;
    QPushButton*  _btnEscuchar;
    QLabel*       _lblArchivo;
    QPushButton*  _btnQuitar;
    QComboBox*    _comboIdioma;
    QSpinBox*     _spinPersonas;

    // Widgets - accion
    QPushButton*  _btnTranscribir;
    QProgressBar* _progress;

    // Widgets - tabs
    QTabWidget*   _tabs;
    QTextEdit*    _areaOriginal;
    QTextEdit*    _areaLimpio;
    QTextEdit*    _areaResumen;
    QTextEdit*    _areaAnalisis;
    QTextEdit*    _areaTraducido;
    QPushButton*  _btnLimpiar;
    QPushButton*  _btnResumir;
    QPushButton*  _btnAnalizar;
    QPushButton*  _btnTraducir;
    QComboBox*    _comboIdiomaTraducir;
    QTextEdit*    _areaChat;
    QLineEdit*    _inputPregunta;
    QPushButton*  _btnPreguntar;

    // Widgets - bottom
    QLabel*       _lblParticipacion;
    QPushButton*  _btnGuardar;

    // Toggle modo Archivo / En Vivo
    QStackedWidget* _stack;
    QPushButton*    _btnModoArchivo;
    QPushButton*    _btnModoEnVivo;
    PanelEnVivo*    _panelEnVivo;
};

} // namespace DIArize::GUI
