#pragma once
#include "gui/PanelBase.h"
#include <QTextEdit>

namespace DIArize::GUI {

// Muestra la ultima transcripcion: hablante por hablante.
// Demuestra: override de virtual pura, polimorfismo concreto.
class PanelTranscripcion : public PanelBase {
public:
    explicit PanelTranscripcion(QWidget* parent = nullptr);
    ~PanelTranscripcion() override;

    void    actualizar(const DIArize::Core::Transcripcion& t) override;
    QString titulo() const override { return "Transcripcion"; }

private:
    QTextEdit* _area;
};

} // namespace DIArize::GUI
