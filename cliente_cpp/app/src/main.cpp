#include <QApplication>
#include "gui/VentanaPrincipal.h"

// QSS — equivalente a CSS para Qt.
// Se aplica una vez y afecta a toda la aplicacion.
static const char* kEstilo = R"(

/* ── Base ────────────────────────────────────────────── */
* {
    font-family: "Segoe UI", Arial, sans-serif;
    font-size: 10pt;
    color: #1F2937;
}
QMainWindow, QDialog { background-color: #F9FAFB; }
QWidget { background-color: transparent; }

/* ── Toolbar ──────────────────────────────────────────── */
QFrame#toolbar {
    background-color: #FFFFFF;
    border-bottom: 1px solid #E5E7EB;
    padding: 4px 8px;
}

/* ── Botones primarios ────────────────────────────────── */
QPushButton {
    background-color: #2563EB;
    color: #FFFFFF;
    border: none;
    border-radius: 6px;
    padding: 7px 20px;
    font-weight: 600;
    min-width: 110px;
}
QPushButton:hover   { background-color: #1D4ED8; }
QPushButton:pressed { background-color: #1E40AF; }
QPushButton:disabled {
    background-color: #D1D5DB;
    color: #9CA3AF;
}

/* ── Boton secundario (Seleccionar) ──────────────────── */
QPushButton#btnSelec {
    background-color: #FFFFFF;
    color: #374151;
    border: 1px solid #D1D5DB;
    font-weight: 500;
}
QPushButton#btnSelec:hover {
    background-color: #F3F4F6;
    border-color: #9CA3AF;
}
QPushButton#btnSelec:pressed { background-color: #E5E7EB; }

/* ── Labels ──────────────────────────────────────────── */
QLabel#lblArchivo {
    color: #374151;
    font-weight: 500;
    padding: 0 6px;
}
QLabel#lblTitulo {
    color: #2563EB;
    font-size: 14pt;
    font-weight: 700;
    padding: 6px 4px;
}

/* ── Tabs ────────────────────────────────────────────── */
QTabWidget::pane {
    border: none;
    background-color: #FFFFFF;
    border-top: 1px solid #E5E7EB;
}
QTabBar {
    background-color: #FFFFFF;
    border-bottom: 1px solid #E5E7EB;
}
QTabBar::tab {
    background-color: transparent;
    color: #6B7280;
    padding: 10px 22px;
    font-weight: 500;
    border-bottom: 2px solid transparent;
    margin-right: 2px;
}
QTabBar::tab:selected {
    color: #2563EB;
    border-bottom: 2px solid #2563EB;
}
QTabBar::tab:hover:!selected { color: #374151; }

/* ── Areas de texto ──────────────────────────────────── */
QTextEdit {
    background-color: #FFFFFF;
    border: none;
    padding: 12px;
    selection-background-color: #DBEAFE;
    color: #1F2937;
}
QListWidget {
    background-color: #FFFFFF;
    border: none;
    padding: 8px;
}
QListWidget::item {
    padding: 10px 8px;
    border-bottom: 1px solid #F3F4F6;
    border-radius: 4px;
}
QListWidget::item:selected {
    background-color: #EFF6FF;
    color: #1E40AF;
}
QListWidget::item:hover:!selected { background-color: #F9FAFB; }

/* ── Barra de progreso ───────────────────────────────── */
QProgressBar {
    border: none;
    background-color: #E5E7EB;
    border-radius: 3px;
    max-height: 5px;
    min-height: 5px;
}
QProgressBar::chunk {
    background-color: #2563EB;
    border-radius: 3px;
}

/* ── Status bar ──────────────────────────────────────── */
QStatusBar {
    background-color: #F3F4F6;
    border-top: 1px solid #E5E7EB;
    color: #6B7280;
    font-size: 9pt;
    padding: 2px 8px;
}
QStatusBar QLabel { background: transparent; }

/* ── Scrollbars ──────────────────────────────────────── */
QScrollBar:vertical {
    background: transparent;
    width: 8px;
    margin: 0;
}
QScrollBar::handle:vertical {
    background: #D1D5DB;
    border-radius: 4px;
    min-height: 24px;
}
QScrollBar::handle:vertical:hover { background: #9CA3AF; }
QScrollBar::add-line:vertical,
QScrollBar::sub-line:vertical { height: 0; }
QScrollBar::add-page:vertical,
QScrollBar::sub-page:vertical { background: transparent; }
QScrollBar:horizontal {
    background: transparent;
    height: 8px;
}
QScrollBar::handle:horizontal {
    background: #D1D5DB;
    border-radius: 4px;
    min-width: 24px;
}
QScrollBar::handle:horizontal:hover { background: #9CA3AF; }
QScrollBar::add-line:horizontal,
QScrollBar::sub-line:horizontal { width: 0; }

)";

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("DIArize");
    app.setApplicationVersion("1.0");
    app.setStyleSheet(kEstilo);

    DIArize::GUI::VentanaPrincipal ventana;
    ventana.show();

    return app.exec();
}
