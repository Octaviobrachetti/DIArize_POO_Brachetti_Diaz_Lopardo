#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QPalette>
#include <QTimer>
#include "gui/PantallaCarga.h"
#include "gui/VentanaPrincipal.h"

// ════════════════════════════════════════════════════════════════════════════
//   TEMA DARK SLATE + CYAN
//   Paleta inspirada en el logo: gris-pizarra profundo + cyan brillante con glow
// ════════════════════════════════════════════════════════════════════════════
static const char* kEstilo = R"(

/* ── Base global ──────────────────────────────────────────────────── */
* {
    font-family: "Segoe UI", "Inter", Arial, sans-serif;
    font-size: 10pt;
    color: #e2e8f0;
}

QMainWindow, QDialog { background-color: #0b1220; }

QMainWindow > QWidget#centralBg {
    background-color: #0b1220;
}

QWidget#centralBg {
    background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1,
        stop:0 #0f172a, stop:1 #0b1220);
}

/* ── Botones primarios (cyan glow) ────────────────────────────────── */
QPushButton {
    background-color: #06b6d4;
    color: #0b1220;
    border: none;
    border-radius: 8px;
    padding: 9px 18px;
    font-weight: 700;
    min-width: 100px;
}
QPushButton:hover:!disabled  { background-color: #22d3ee; }
QPushButton:pressed          { background-color: #0891b2; }
QPushButton:disabled {
    background-color: #1f2937;
    color: #475569;
}

/* ── Boton transcribir (destacado: gradiente cyan) ────────────────── */
QPushButton#btnTranscribir {
    font-size: 11pt;
    padding: 13px 28px;
    border-radius: 10px;
    min-width: 200px;
    background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0,
        stop:0 #06b6d4, stop:1 #0ea5e9);
    color: #042f2e;
}
QPushButton#btnTranscribir:hover:!disabled {
    background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0,
        stop:0 #22d3ee, stop:1 #38bdf8);
}

/* ── Boton cargar (outline cyan) ──────────────────────────────────── */
QPushButton#btnCargar {
    background-color: transparent;
    color: #22d3ee;
    border: 1.5px solid #0891b2;
    font-weight: 700;
}
QPushButton#btnCargar:hover:!disabled {
    background-color: rgba(34, 211, 238, 30);
    border-color: #22d3ee;
}
QPushButton#btnCargar:disabled {
    color: #475569;
    border-color: #1f2937;
}

/* ── Boton grabar (rojo glow) ─────────────────────────────────────── */
QPushButton#btnGrabar {
    background-color: #ef4444;
    color: #ffffff;
}
QPushButton#btnGrabar:hover:!disabled { background-color: #f87171; }
QPushButton#btnGrabar:pressed         { background-color: #dc2626; }

/* ── Boton escuchar (verde-esmeralda) ─────────────────────────────── */
QPushButton#btnEscuchar {
    background-color: #10b981;
    color: #042f2e;
}
QPushButton#btnEscuchar:hover:!disabled { background-color: #34d399; }
QPushButton#btnEscuchar:pressed         { background-color: #059669; }

/* ── Boton quitar (icono X transparente) ──────────────────────────── */
QPushButton#btnQuitar {
    background-color: transparent;
    color: #64748b;
    border: none;
    font-size: 13pt;
    font-weight: bold;
    min-width: 0;
    max-width: 28px;
    padding: 0;
}
QPushButton#btnQuitar:hover { color: #ef4444; }

/* ── Botones IA dentro de tabs (violeta-cyan gradiente) ──────────── */
QPushButton#btnIA {
    background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0,
        stop:0 #8b5cf6, stop:1 #06b6d4);
    color: #ffffff;
    padding: 11px 24px;
    font-size: 10.5pt;
    border-radius: 9px;
}
QPushButton#btnIA:hover:!disabled {
    background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0,
        stop:0 #a78bfa, stop:1 #22d3ee);
}

/* ── Boton guardar ────────────────────────────────────────────────── */
QPushButton#btnGuardar {
    background-color: #10b981;
    color: #042f2e;
}
QPushButton#btnGuardar:hover:!disabled { background-color: #34d399; }

/* ── Labels ───────────────────────────────────────────────────────── */
QLabel { background-color: transparent; color: #cbd5e1; }
QLabel#lblArchivo {
    color: #94a3b8;
    padding: 0 8px;
    font-style: italic;
}
QLabel#lblParticipacion {
    color: #94a3b8;
    font-size: 9.5pt;
    padding: 0 4px;
}

/* ── Tabs ─────────────────────────────────────────────────────────── */
QTabWidget::pane {
    border: 1px solid #1f2937;
    background-color: #111827;
    border-radius: 12px;
    top: -1px;
}
QTabBar { background-color: transparent; }
QTabBar::tab {
    background: transparent;
    color: #64748b;
    padding: 11px 26px;
    font-weight: 600;
    border: none;
    margin-right: 4px;
}
QTabBar::tab:selected {
    color: #22d3ee;
    background: #111827;
    border-top-left-radius: 10px;
    border-top-right-radius: 10px;
    border: 1px solid #1f2937;
    border-bottom: 1px solid #111827;
}
QTabBar::tab:hover:!selected {
    color: #cbd5e1;
    background: rgba(34, 211, 238, 14);
    border-top-left-radius: 10px;
    border-top-right-radius: 10px;
}

/* ── Areas de texto ───────────────────────────────────────────────── */
QTextEdit {
    background-color: #0f172a;
    border: 1px solid #1f2937;
    border-radius: 10px;
    padding: 14px;
    color: #e2e8f0;
    selection-background-color: #0e7490;
    selection-color: #ecfeff;
    font-size: 10.5pt;
}
QTextEdit:focus { border-color: #06b6d4; }

QLineEdit {
    background-color: #0f172a;
    border: 1px solid #1f2937;
    border-radius: 8px;
    padding: 9px 13px;
    color: #e2e8f0;
}
QLineEdit:focus { border-color: #06b6d4; }
QLineEdit::placeholder { color: #475569; }

/* ── Combos y spinbox ─────────────────────────────────────────────── */
QComboBox, QSpinBox {
    background-color: #0f172a;
    border: 1px solid #334155;
    border-radius: 7px;
    padding: 7px 12px;
    color: #e2e8f0;
    min-width: 80px;
}
QComboBox:hover, QSpinBox:hover { border-color: #06b6d4; }
QComboBox:focus, QSpinBox:focus { border-color: #22d3ee; }
QComboBox::drop-down {
    border: none;
    width: 22px;
}
QComboBox::down-arrow { width: 0; height: 0; }
QComboBox QAbstractItemView {
    background-color: #0f172a;
    border: 1px solid #1f2937;
    border-radius: 8px;
    selection-background-color: #0e7490;
    selection-color: #ecfeff;
    color: #cbd5e1;
    padding: 4px;
    outline: 0;
}
QSpinBox::up-button, QSpinBox::down-button {
    width: 18px;
    background: transparent;
    border: none;
}

/* ── Barra de progreso ────────────────────────────────────────────── */
QProgressBar {
    border: none;
    background-color: #1f2937;
    border-radius: 4px;
    min-height: 7px;
    max-height: 7px;
}
QProgressBar::chunk {
    background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0,
        stop:0 #06b6d4, stop:1 #22d3ee);
    border-radius: 4px;
}

/* ── Status bar ───────────────────────────────────────────────────── */
QStatusBar {
    background-color: #0f172a;
    border-top: 1px solid #1f2937;
    color: #94a3b8;
    font-size: 9.5pt;
    padding: 4px 14px;
}
QStatusBar::item { border: none; }
QStatusBar QLabel { background: transparent; }

/* ── Scrollbars ───────────────────────────────────────────────────── */
QScrollBar:vertical {
    background: transparent;
    width: 10px;
    margin: 4px 0;
}
QScrollBar::handle:vertical {
    background: #334155;
    border-radius: 5px;
    min-height: 30px;
}
QScrollBar::handle:vertical:hover { background: #06b6d4; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }

QScrollBar:horizontal { background: transparent; height: 10px; margin: 0 4px; }
QScrollBar::handle:horizontal {
    background: #334155;
    border-radius: 5px;
    min-width: 30px;
}
QScrollBar::handle:horizontal:hover { background: #06b6d4; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }

/* ── Toggle de modo Archivo / En Vivo ─────────────────────────────── */
QPushButton#btnToggleActivo {
    background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0,
        stop:0 #06b6d4, stop:1 #0ea5e9);
    color: #042f2e;
    border: none;
    border-radius: 11px;
    padding: 11px 30px;
    font-size: 11pt;
    font-weight: 800;
    min-width: 180px;
}
QPushButton#btnToggleActivo:hover {
    background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0,
        stop:0 #22d3ee, stop:1 #38bdf8);
}

QPushButton#btnToggleInactivo {
    background-color: #111827;
    color: #94a3b8;
    border: 1.5px solid #334155;
    border-radius: 11px;
    padding: 11px 30px;
    font-size: 11pt;
    font-weight: 700;
    min-width: 180px;
}
QPushButton#btnToggleInactivo:hover {
    background-color: #1f2937;
    color: #cbd5e1;
    border-color: #06b6d4;
}

/* ── Panel En Vivo ────────────────────────────────────────────── */
QPushButton#btnIniciarVivo {
    background-color: #ef4444;
    color: #ffffff;
    font-size: 14pt;
    font-weight: 800;
    border-radius: 14px;
    padding: 16px 28px;
    letter-spacing: 1px;
}
QPushButton#btnIniciarVivo:hover { background-color: #f87171; }
QPushButton#btnIniciarVivo:disabled {
    background-color: #1f2937;
    color: #475569;
}

QPushButton#btnDetenerVivo {
    background-color: #334155;
    color: #f1f5f9;
    font-size: 14pt;
    font-weight: 800;
    border-radius: 14px;
    padding: 16px 28px;
    letter-spacing: 1px;
}
QPushButton#btnDetenerVivo:hover { background-color: #475569; }

QPushButton#btnSecundario {
    background-color: transparent;
    color: #94a3b8;
    border: 1px solid #334155;
    font-weight: 600;
    border-radius: 8px;
    padding: 7px 16px;
    min-width: 0;
}
QPushButton#btnSecundario:hover:!disabled {
    color: #22d3ee;
    border-color: #06b6d4;
    background-color: rgba(34, 211, 238, 14);
}
QPushButton#btnSecundario:disabled { color: #475569; border-color: #1f2937; }

QPushButton#btnDiarizar {
    background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0,
        stop:0 #8b5cf6, stop:1 #06b6d4);
    color: #ffffff;
    font-size: 11pt;
    font-weight: 700;
    border-radius: 10px;
    padding: 12px 28px;
    min-width: 320px;
}
QPushButton#btnDiarizar:hover {
    background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0,
        stop:0 #a78bfa, stop:1 #22d3ee);
}

QLabel#lblEstadoVivo {
    color: #94a3b8;
    font-size: 10pt;
}
QLabel#lblTiempoVivo {
    color: #f87171;
    font-size: 20pt;
    font-weight: 800;
    font-family: "JetBrains Mono", "Consolas", monospace;
    letter-spacing: 2px;
}
QTextEdit#areaVivo {
    font-size: 12pt;
    line-height: 1.6;
    background-color: #0f172a;
    border: 1px solid #1f2937;
}

/* ── Tooltips ─────────────────────────────────────────────────────── */
QToolTip {
    background-color: #1e293b;
    color: #e2e8f0;
    border: 1px solid #06b6d4;
    border-radius: 6px;
    padding: 7px 11px;
}

/* ── MessageBox ───────────────────────────────────────────────────── */
QMessageBox { background-color: #0f172a; }
QMessageBox QLabel { color: #e2e8f0; }
)";

// ────────────────────────────────────────────────────────────────────
//   main: splash screen primero, ventana principal despues
// ────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("DIArize");
    app.setApplicationVersion("1.0");
    app.setStyle("Fusion");

    // Palette dark slate
    QPalette palette;
    palette.setColor(QPalette::Window,           QColor("#0b1220"));
    palette.setColor(QPalette::WindowText,       QColor("#e2e8f0"));
    palette.setColor(QPalette::Base,             QColor("#0f172a"));
    palette.setColor(QPalette::AlternateBase,    QColor("#111827"));
    palette.setColor(QPalette::Text,             QColor("#e2e8f0"));
    palette.setColor(QPalette::Button,           QColor("#06b6d4"));
    palette.setColor(QPalette::ButtonText,       QColor("#0b1220"));
    palette.setColor(QPalette::ToolTipBase,      QColor("#1e293b"));
    palette.setColor(QPalette::ToolTipText,      QColor("#e2e8f0"));
    palette.setColor(QPalette::Highlight,        QColor("#06b6d4"));
    palette.setColor(QPalette::HighlightedText,  QColor("#042f2e"));
    palette.setColor(QPalette::PlaceholderText,  QColor("#475569"));
    QApplication::setPalette(palette);
    app.setStyleSheet(kEstilo);

    if (QFile::exists(":/diarize_logo.png"))
        app.setWindowIcon(QIcon(":/diarize_logo.png"));

    // ── Splash screen ─────────────────────────────────────────────
    auto* splash   = new DIArize::GUI::PantallaCarga();
    auto* ventana  = new DIArize::GUI::VentanaPrincipal();

    QObject::connect(splash, &DIArize::GUI::PantallaCarga::terminado,
                     [ventana]() { ventana->show(); });

    splash->mostrarYCerrar(2400);

    int rc = app.exec();
    delete ventana;
    return rc;
}
