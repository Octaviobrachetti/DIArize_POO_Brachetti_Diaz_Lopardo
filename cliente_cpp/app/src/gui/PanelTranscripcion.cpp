#include "gui/PanelTranscripcion.h"
#include <QVBoxLayout>

namespace DIArize::GUI {

PanelTranscripcion::PanelTranscripcion(QWidget* parent)
    : PanelBase(parent), _area(new QTextEdit(this))
{
    _area->setReadOnly(true);
    _area->setPlaceholderText("Selecciona un archivo de audio y presiona Transcribir...");
    auto* lay = new QVBoxLayout(this);
    lay->addWidget(_area);
    setLayout(lay);
}

PanelTranscripcion::~PanelTranscripcion() = default;

void PanelTranscripcion::actualizar(const DIArize::Core::Transcripcion& t) {
    if (t.getSegmentos().empty()) {
        _area->setPlainText(QString::fromStdString(t.getTexto()));
        return;
    }

    QString html = "<html><body style='font-family:sans-serif;font-size:11pt'>";
    for (const auto& seg : t.getSegmentos()) {
        double ini = seg.getInicio();
        double fin = seg.getFin();
        html += QString(
            "<p><span style='color:#1565c0;font-weight:bold'>[%1]</span> "
            "<span style='color:#555;font-size:9pt'>%2s – %3s</span><br>"
            "%4</p>")
            .arg(QString::fromStdString(seg.getSpeaker()))
            .arg(ini, 0, 'f', 1)
            .arg(fin, 0, 'f', 1)
            .arg(QString::fromStdString(seg.getTexto()));
    }
    html += "</body></html>";
    _area->setHtml(html);
}

} // namespace DIArize::GUI
