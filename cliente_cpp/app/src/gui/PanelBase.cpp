#include "gui/PanelBase.h"

namespace DIArize::GUI {

PanelBase::PanelBase(QWidget* parent) : QWidget(parent) {}

PanelBase::~PanelBase() = default;  // vtable anclada a esta translation unit

} // namespace DIArize::GUI
