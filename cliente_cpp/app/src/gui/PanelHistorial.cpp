#include "gui/PanelHistorial.h"
#include <QFileInfo>
#include <QVBoxLayout>

namespace DIArize::GUI {

PanelHistorial::PanelHistorial(QWidget* parent)
    : PanelBase(parent), _lista(new QListWidget(this))
{
    auto* lay = new QVBoxLayout(this);
    lay->addWidget(_lista);
    setLayout(lay);
}

PanelHistorial::~PanelHistorial() = default;

void PanelHistorial::actualizar(const DIArize::Core::Transcripcion& t) {
    QString nombre = QFileInfo(QString::fromStdString(t.getArchivo())).fileName();
    int     nSeg   = static_cast<int>(t.getSegmentos().size());
    QString fecha  = QString::fromStdString(t.getFecha());

    QString texto = QString("%1  |  %2 segmentos  |  %3")
        .arg(nombre).arg(nSeg).arg(fecha);

    _lista->insertItem(0, texto);   // mas reciente arriba
    _lista->setCurrentRow(0);
}

} // namespace DIArize::GUI
