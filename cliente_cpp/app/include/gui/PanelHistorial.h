#pragma once
#include "gui/PanelBase.h"
#include <QListWidget>
#include <vector>
#include "nucleo/Transcripcion.h"

namespace DIArize::GUI {

// Muestra el historial de todas las transcripciones de la sesion.
// Demuestra: segundo override concreto de PanelBase (polimorfismo).
class PanelHistorial : public PanelBase {
public:
    explicit PanelHistorial(QWidget* parent = nullptr);
    ~PanelHistorial() override;

    void    actualizar(const DIArize::Core::Transcripcion& t) override;
    QString titulo() const override { return "Historial"; }

private:
    QListWidget* _lista;
};

} // namespace DIArize::GUI
