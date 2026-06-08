#pragma once
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QTextEdit>
#include <QTimer>
#include <QWidget>

#include "red/ClienteStreaming.h"

namespace DIArize::GUI {

/**
 * Panel del modo "En Vivo": permite iniciar/detener una sesion
 * de transcripcion en streaming via AssemblyAI Universal Streaming.
 */
class PanelEnVivo : public QWidget {
    Q_OBJECT
public:
    explicit PanelEnVivo(QWidget* parent = nullptr);

signals:
    // Emitido cuando el usuario quiere mandar el WAV recien grabado
    // al modo Archivo para procesarlo con diarizacion + IA.
    void procesarConArchivo(const QString& rutaWav);

private slots:
    void _toggleSesion();
    void _onConectado();
    void _onParcial(const QString& texto);
    void _onFinal(const QString& texto);
    void _onError(const QString& mensaje);
    void _onFinalizado();
    void _tickTimer();
    void _copiar();
    void _limpiar();
    void _procesarConDiarizacion();

private:
    void _setEstado(const QString& msg);
    void _pedirTokenYIniciar();

    DIArize::Red::ClienteStreaming* _streaming;

    // UI
    QPushButton* _btnSesion;
    QPushButton* _btnCopiar;
    QPushButton* _btnLimpiar;
    QPushButton* _btnDiarizar;
    QLabel*      _lblEstado;
    QLabel*      _lblTiempo;
    QTextEdit*   _area;

    QString      _textoFinal;        // acumulado de turnos finalizados
    QString      _ultimoParcial;     // ultimo parcial (se sobrescribe)
    QTimer*      _timer;
    int          _segundos = 0;
    bool         _activo   = false;
};

} // namespace DIArize::GUI
