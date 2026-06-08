#pragma once
#include <QWidget>
#include <QString>
#include "nucleo/Transcripcion.h"

namespace DIArize::GUI {

// Clase abstracta para paneles de la interfaz grafica.
// Demuestra: herencia de QWidget + funcion virtual pura propia.
// No se puede instanciar directamente.
class PanelBase : public QWidget {
public:
    explicit PanelBase(QWidget* parent = nullptr);
    ~PanelBase() override;  // definido en .cpp para anclar la vtable

    // Funcion virtual pura: cada panel decide como mostrar la transcripcion
    virtual void actualizar(const DIArize::Core::Transcripcion& t) = 0;

    // Funcion virtual con implementacion: los paneles pueden personalizar su titulo
    virtual QString titulo() const { return "Panel"; }
};

} // namespace DIArize::GUI
