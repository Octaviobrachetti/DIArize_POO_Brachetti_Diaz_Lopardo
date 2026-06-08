#pragma once
#include <QCloseEvent>
#include <QLabel>
#include <QMainWindow>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QTabWidget>
#include <QTimer>
#include <memory>
#include <vector>

#include "gui/PanelBase.h"
#include "nucleo/IObservador.h"
#include "nucleo/ITranscriptor.h"
#include "nucleo/Repositorio.h"

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
    void _seleccionarArchivo();
    void _transcribir();
    void _verificarServidor();   // llamado por QTimer hasta que el server este listo

private:
    void _iniciarServidor();
    void _actualizarPaneles(const DIArize::Core::Transcripcion& t);

    // Backend
    std::unique_ptr<DIArize::Core::ITranscriptor> _transcriptor;
    DIArize::Core::Repositorio                    _repositorio;
    QString                                       _rutaAudio;
    std::vector<PanelBase*>                       _paneles;

    // Proceso del servidor Flask (arrancado automaticamente)
    QProcess* _serverProcess   = nullptr;
    QTimer*   _startupTimer    = nullptr;
    int       _startupAttempts = 0;

    // Widgets
    QLabel*       _lblArchivo;
    QPushButton*  _btnSelec;
    QProgressBar* _progress;
    QPushButton*  _btnTranscribir;
    QTabWidget*   _tabs;
};

} // namespace DIArize::GUI
