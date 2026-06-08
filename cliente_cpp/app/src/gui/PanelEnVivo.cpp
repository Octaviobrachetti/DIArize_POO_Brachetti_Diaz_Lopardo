#include "gui/PanelEnVivo.h"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QScrollBar>
#include <QStyle>
#include <QVBoxLayout>

namespace DIArize::GUI {

PanelEnVivo::PanelEnVivo(QWidget* parent) : QWidget(parent) {
    _streaming = new DIArize::Red::ClienteStreaming(this);

    // ── UI ────────────────────────────────────────────────────────
    _btnSesion = new QPushButton("🔴  INICIAR EN VIVO", this);
    _btnSesion->setObjectName("btnIniciarVivo");
    _btnSesion->setMinimumHeight(56);

    _btnCopiar   = new QPushButton("📋 Copiar texto", this);
    _btnLimpiar  = new QPushButton("🗑 Limpiar",       this);
    _btnDiarizar = new QPushButton("🎭 Procesar con diarización + IA", this);
    _btnCopiar  ->setObjectName("btnSecundario");
    _btnLimpiar ->setObjectName("btnSecundario");
    _btnDiarizar->setObjectName("btnDiarizar");
    _btnCopiar  ->setEnabled(false);
    _btnLimpiar ->setEnabled(false);
    _btnDiarizar->setVisible(false);   // aparece solo al detener una sesion con audio
    _btnDiarizar->setToolTip(
        "Manda el audio grabado al modo Archivo para diarizarlo\n"
        "(Persona 1, Persona 2, ...) y procesarlo con Gemini.");

    _lblEstado = new QLabel("Listo. Cuando inicies, te conecto en vivo con AssemblyAI.", this);
    _lblEstado->setObjectName("lblEstadoVivo");

    _lblTiempo = new QLabel("00:00", this);
    _lblTiempo->setObjectName("lblTiempoVivo");

    _area = new QTextEdit(this);
    _area->setReadOnly(true);
    _area->setPlaceholderText("Acá va a aparecer la transcripcion en tiempo real…");
    _area->setObjectName("areaVivo");

    auto* fila1 = new QHBoxLayout();
    fila1->addWidget(_btnSesion, 1);
    fila1->addSpacing(20);
    fila1->addWidget(_lblTiempo);

    auto* fila2 = new QHBoxLayout();
    fila2->addWidget(_lblEstado, 1);
    fila2->addWidget(_btnCopiar);
    fila2->addWidget(_btnLimpiar);

    auto* filaDiarizar = new QHBoxLayout();
    filaDiarizar->addStretch(1);
    filaDiarizar->addWidget(_btnDiarizar);
    filaDiarizar->addStretch(1);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 20, 20, 20);
    root->setSpacing(14);
    root->addLayout(fila1);
    root->addLayout(fila2);
    root->addWidget(_area, 1);
    root->addLayout(filaDiarizar);

    // ── Timer cronometro ──────────────────────────────────────────
    _timer = new QTimer(this);
    _timer->setInterval(1000);

    // ── Connections ───────────────────────────────────────────────
    connect(_btnSesion,   &QPushButton::clicked, this, &PanelEnVivo::_toggleSesion);
    connect(_btnCopiar,   &QPushButton::clicked, this, &PanelEnVivo::_copiar);
    connect(_btnLimpiar,  &QPushButton::clicked, this, &PanelEnVivo::_limpiar);
    connect(_btnDiarizar, &QPushButton::clicked, this, &PanelEnVivo::_procesarConDiarizacion);
    connect(_timer,       &QTimer::timeout,      this, &PanelEnVivo::_tickTimer);

    connect(_streaming, &DIArize::Red::ClienteStreaming::conectado,
            this, &PanelEnVivo::_onConectado);
    connect(_streaming, &DIArize::Red::ClienteStreaming::textoParcial,
            this, &PanelEnVivo::_onParcial);
    connect(_streaming, &DIArize::Red::ClienteStreaming::textoFinal,
            this, &PanelEnVivo::_onFinal);
    connect(_streaming, &DIArize::Red::ClienteStreaming::error,
            this, &PanelEnVivo::_onError);
    connect(_streaming, &DIArize::Red::ClienteStreaming::finalizado,
            this, &PanelEnVivo::_onFinalizado);
}

// ── Sesion ──────────────────────────────────────────────────────────
void PanelEnVivo::_toggleSesion() {
    if (_activo) {
        _setEstado("Cerrando sesion...");
        _streaming->detener();
        return;
    }
    _pedirTokenYIniciar();
}

void PanelEnVivo::_pedirTokenYIniciar() {
    _setEstado("Pidiendo token a AssemblyAI...");
    _btnSesion->setEnabled(false);

    auto* mgr = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl("http://localhost:8765/streaming-token"));
    QNetworkReply* reply = mgr->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, mgr]() {
        QByteArray data = reply->readAll();
        reply->deleteLater();
        mgr->deleteLater();

        QJsonObject obj = QJsonDocument::fromJson(data).object();
        if (obj.contains("token")) {
            QString tok = obj["token"].toString();
            if (tok.isEmpty()) {
                _setEstado("Error: token vacio.");
                _btnSesion->setEnabled(true);
                return;
            }
            _setEstado("Conectando al streaming de AssemblyAI...");
            _streaming->iniciar(tok);
        } else {
            QString err = obj.value("error").toString("No se pudo obtener token.");
            _setEstado("Error: " + err);
            _btnSesion->setEnabled(true);
        }
    });
}

void PanelEnVivo::_onConectado() {
    _activo = true;
    _segundos = 0;
    _lblTiempo->setText("00:00");
    _timer->start();
    _btnSesion->setEnabled(true);
    _btnSesion->setText("⏹  DETENER");
    _btnSesion->setObjectName("btnDetenerVivo");
    _btnSesion->style()->unpolish(_btnSesion);
    _btnSesion->style()->polish(_btnSesion);
    _btnDiarizar->setVisible(false);    // reset
    _setEstado("🔴 Conectado. Hablá nomás...");
}

void PanelEnVivo::_onParcial(const QString& texto) {
    _ultimoParcial = texto;
    // Pinta el acumulado de finales + el parcial actual (gris/cursiva)
    QString html = _textoFinal.toHtmlEscaped().replace("\n", "<br>");
    if (!html.isEmpty() && !_ultimoParcial.isEmpty()) html += " ";
    html += QString("<span style='color:#94a3b8;font-style:italic'>%1</span>")
              .arg(_ultimoParcial.toHtmlEscaped());
    _area->setHtml(html);

    // Auto-scroll
    auto bar = _area->verticalScrollBar();
    bar->setValue(bar->maximum());
}

void PanelEnVivo::_onFinal(const QString& texto) {
    if (!_textoFinal.isEmpty()) _textoFinal += " ";
    _textoFinal += texto;
    _ultimoParcial.clear();
    _area->setPlainText(_textoFinal);

    auto bar = _area->verticalScrollBar();
    bar->setValue(bar->maximum());

    _btnCopiar ->setEnabled(true);
    _btnLimpiar->setEnabled(true);
}

void PanelEnVivo::_onError(const QString& mensaje) {
    _setEstado("⚠ Error: " + mensaje);
    // Importante: NO mostrar modal, sino se traba la UI con varios popups
    // si AssemblyAI manda errores en cadena. Solo se ve en el label.
    _streaming->detener();
}

void PanelEnVivo::_onFinalizado() {
    _activo = false;
    _timer->stop();
    _btnSesion->setText("🔴  INICIAR EN VIVO");
    _btnSesion->setObjectName("btnIniciarVivo");
    _btnSesion->style()->unpolish(_btnSesion);
    _btnSesion->style()->polish(_btnSesion);
    _btnSesion->setEnabled(true);
    _setEstado(QString("Sesion cerrada. %1 caracteres transcriptos.")
                 .arg(_textoFinal.length()));

    // Mostrar boton "Procesar con diarización" si hay un WAV con audio suficiente
    QString ruta = _streaming->rutaWav();
    _btnDiarizar->setVisible(!ruta.isEmpty());
}

void PanelEnVivo::_procesarConDiarizacion() {
    QString ruta = _streaming->rutaWav();
    if (ruta.isEmpty()) return;
    emit procesarConArchivo(ruta);
}

void PanelEnVivo::_tickTimer() {
    _segundos++;
    int m = _segundos / 60, s = _segundos % 60;
    _lblTiempo->setText(QString("%1:%2")
        .arg(m, 2, 10, QLatin1Char('0'))
        .arg(s, 2, 10, QLatin1Char('0')));
}

void PanelEnVivo::_copiar() {
    QApplication::clipboard()->setText(_textoFinal);
    _setEstado("Texto copiado al portapapeles ✓");
}

void PanelEnVivo::_limpiar() {
    _textoFinal.clear();
    _ultimoParcial.clear();
    _area->clear();
    _btnCopiar ->setEnabled(false);
    _btnLimpiar->setEnabled(false);
    _setEstado("Listo para una nueva sesion.");
}

void PanelEnVivo::_setEstado(const QString& msg) {
    _lblEstado->setText(msg);
}

} // namespace DIArize::GUI
