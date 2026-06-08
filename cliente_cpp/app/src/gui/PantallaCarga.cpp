#include "gui/PantallaCarga.h"

#include <QApplication>
#include <QFile>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QRadialGradient>
#include <QScreen>
#include <QVBoxLayout>

namespace DIArize::GUI {

PantallaCarga::PantallaCarga(QWidget* parent) : QWidget(parent) {
    // Ventana sin marco, transparente para poder pintar el gradient
    setWindowFlags(Qt::SplashScreen | Qt::FramelessWindowHint
                   | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(640, 420);

    // ── Logo (imagen del recurso, o fallback texto) ─────────────────
    _lblLogo = new QLabel(this);
    _lblLogo->setAlignment(Qt::AlignCenter);

    QPixmap logo;
    if (QFile::exists(":/diarize_logo.png")) {
        logo.load(":/diarize_logo.png");
    }
    // Si la imagen es mas grande que el placeholder (>32x32), la usamos.
    // Si no, fallback de texto con efecto.
    if (!logo.isNull() && logo.width() > 32 && logo.height() > 32) {
        logo = logo.scaled(280, 280, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        _lblLogo->setPixmap(logo);
    } else {
        _lblLogo->setText("dIArize");
        _lblLogo->setStyleSheet(
            "color: #22d3ee; font-size: 56pt; font-weight: 900;"
            "letter-spacing: 2px;");
        auto* glowLogo = new QGraphicsDropShadowEffect(this);
        glowLogo->setBlurRadius(60);
        glowLogo->setColor(QColor(6, 182, 212, 220));
        glowLogo->setOffset(0, 0);
        _lblLogo->setGraphicsEffect(glowLogo);
    }

    // ── Titulo + subtitulo ──────────────────────────────────────────
    _lblTitulo = new QLabel("DIArize", this);
    _lblTitulo->setAlignment(Qt::AlignCenter);
    _lblTitulo->setStyleSheet(
        "color: #f0f9ff; font-size: 22pt; font-weight: 800;"
        "letter-spacing: 6px;");

    _lblSubtitulo = new QLabel("Transcripcion + Diarizacion + IA", this);
    _lblSubtitulo->setAlignment(Qt::AlignCenter);
    _lblSubtitulo->setStyleSheet(
        "color: #94a3b8; font-size: 10pt; letter-spacing: 4px;");

    _lblEstado = new QLabel("Iniciando", this);
    _lblEstado->setAlignment(Qt::AlignCenter);
    _lblEstado->setStyleSheet(
        "color: #06b6d4; font-size: 10pt; font-weight: 600;"
        "letter-spacing: 2px;");

    // Glow sutil al texto del titulo
    auto* glow = new QGraphicsDropShadowEffect(this);
    glow->setBlurRadius(40);
    glow->setColor(QColor(34, 211, 238, 180));
    glow->setOffset(0, 0);
    _lblTitulo->setGraphicsEffect(glow);

    // ── Layout ──────────────────────────────────────────────────────
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(40, 40, 40, 40);
    lay->setSpacing(0);
    lay->addStretch(1);
    lay->addWidget(_lblLogo);
    lay->addSpacing(8);
    lay->addWidget(_lblTitulo);
    lay->addSpacing(2);
    lay->addWidget(_lblSubtitulo);
    lay->addStretch(1);
    lay->addWidget(_lblEstado);

    // ── Animacion de puntos ─────────────────────────────────────────
    connect(&_timer, &QTimer::timeout, this, [this]() {
        _dotPulse = (_dotPulse + 1) % 4;
        QString puntos(_dotPulse, '.');
        _lblEstado->setText("Iniciando" + puntos);
    });
    _timer.start(350);

    // Centrar en la pantalla
    if (QScreen* sc = QApplication::primaryScreen()) {
        QRect rg = sc->availableGeometry();
        move(rg.center() - rect().center());
    }
}

void PantallaCarga::mostrarYCerrar(int msMostrar) {
    show();
    raise();
    QTimer::singleShot(msMostrar, this, &PantallaCarga::_iniciarFadeOut);
}

void PantallaCarga::_iniciarFadeOut() {
    auto* fade = new QPropertyAnimation(this, "windowOpacity");
    fade->setDuration(400);
    fade->setStartValue(1.0);
    fade->setEndValue(0.0);
    connect(fade, &QPropertyAnimation::finished, this, [this]() {
        _timer.stop();
        emit terminado();
        close();
        deleteLater();
    });
    fade->start(QAbstractAnimation::DeleteWhenStopped);
}

void PantallaCarga::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // ── Fondo: gradient gris-azulado tipo el logo ──────────────────
    QRectF r = rect();
    QLinearGradient bg(r.topLeft(), r.bottomRight());
    bg.setColorAt(0.0,  QColor("#1e293b"));   // slate-800 arriba izq
    bg.setColorAt(0.5,  QColor("#0f172a"));   // slate-900 medio
    bg.setColorAt(1.0,  QColor("#020617"));   // slate-950 abajo der
    QPainterPath path;
    path.addRoundedRect(r, 16, 16);
    p.fillPath(path, bg);

    // ── Glow radial cyan detras del logo ────────────────────────────
    QRadialGradient halo(r.center().x(), r.center().y() - 30, 280);
    halo.setColorAt(0.0, QColor(34, 211, 238, 90));
    halo.setColorAt(0.5, QColor(34, 211, 238, 30));
    halo.setColorAt(1.0, QColor(34, 211, 238, 0));
    p.fillPath(path, halo);

    // ── Borde fino con glow ────────────────────────────────────────
    QPen borde(QColor(34, 211, 238, 70), 1.2);
    p.setPen(borde);
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(r.adjusted(0.5, 0.5, -0.5, -0.5), 16, 16);
}

} // namespace DIArize::GUI
