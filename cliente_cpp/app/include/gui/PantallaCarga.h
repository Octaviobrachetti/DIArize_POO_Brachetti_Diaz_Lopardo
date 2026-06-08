#pragma once
#include <QLabel>
#include <QPropertyAnimation>
#include <QTimer>
#include <QWidget>

namespace DIArize::GUI {

/**
 * Splash screen profesional. Frameless, gradient gris-cyan,
 * logo centrado, mensaje "Iniciando..." y fade-out al cerrar.
 */
class PantallaCarga : public QWidget {
    Q_OBJECT
public:
    explicit PantallaCarga(QWidget* parent = nullptr);

    /// Muestra y dispara fade-out automatico despues de msMostrar.
    void mostrarYCerrar(int msMostrar = 2200);

signals:
    void terminado();

protected:
    void paintEvent(QPaintEvent* e) override;

private:
    void _iniciarFadeOut();

    QLabel* _lblLogo;
    QLabel* _lblTitulo;
    QLabel* _lblSubtitulo;
    QLabel* _lblEstado;
    QTimer  _timer;
    int     _dotPulse = 0;
};

} // namespace DIArize::GUI
