#include "style.hpp"
#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QPalette>

void applyCalculatorStyle(QApplication& application) {
    application.setStyle("Fusion");
    QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    font.setPointSize(12);
    application.setFont(font);
    // A fixed light palette keeps system dark mode from changing table text,
    // dialogs, input selection or otherwise overriding the application theme.
    QPalette palette;
    palette.setColor(QPalette::Window, QColor("#f5faf7"));
    palette.setColor(QPalette::WindowText, QColor("#26382f"));
    palette.setColor(QPalette::Base, QColor("#ffffff"));
    palette.setColor(QPalette::AlternateBase, QColor("#f5f9f6"));
    palette.setColor(QPalette::Text, QColor("#26382f"));
    palette.setColor(QPalette::Button, QColor("#ffffff"));
    palette.setColor(QPalette::ButtonText, QColor("#26382f"));
    palette.setColor(QPalette::Highlight, QColor("#dfeee4"));
    palette.setColor(QPalette::HighlightedText, QColor("#206443"));
    palette.setColor(QPalette::PlaceholderText, QColor("#859a8c"));
    application.setPalette(palette);
    application.setStyleSheet(QString::fromUtf8(R"QSS(
        QMainWindow, QWidget[role="canvas"] { background: #f5faf7; }
        QWidget { color: #26382f; }
        QLabel { background: transparent; }
        QFrame[role="sidebar"] { background: #ffffff; border-right: 1px solid #dfe8e2; }
        QLabel[role="brand"] { color: #203e2e; font-size: 23px; font-weight: 700; }
        QLabel[role="brandMark"] { background: #217a55; color: white; border-radius: 14px;
            font-size: 24px; font-weight: 600; }
        QLabel[role="eyebrow"] { color: #65816f; font-size: 11px; font-weight: 600; }
        QLabel[role="pageTitle"] { font-size: 28px; font-weight: 700; color: #26382f; }
        QLabel[role="subtitle"], QLabel[role="muted"] { color: #697e71; font-size: 12px; }
        QLabel[role="sectionTitle"] { font-size: 15px; font-weight: 600; }
        QLabel[role="fieldLabel"] { font-size: 12px; color: #526d5c; font-weight: 600; }
        QLabel[role="badge"] { color: #27784f; background: #eaf5ed; border-radius: 12px;
            padding: 6px 12px; font-size: 11px; }
        QPushButton { background: #ffffff; border: 1px solid #dce7df;
            border-radius: 9px; padding: 9px 13px; font-size: 12px; }
        QPushButton:hover { background: #f0f8f2; border-color: #a0c9af; }
        QPushButton:pressed { background: #dfefe4; }
        QPushButton:disabled { color: #9baa9f; background: #f2f6f3; border-color: #e3ebe5; }
        QPushButton[role="primary"], QPushButton[role="equals"] { background: #217a55; color: white;
            border: 1px solid #217a55; font-weight: 600; }
        QPushButton[role="primary"]:hover, QPushButton[role="equals"]:hover { background: #1a6546; }
        QPushButton[role="primary"]:pressed, QPushButton[role="equals"]:pressed { background: #155238; }
        QPushButton[role="navigation"] { text-align: left; padding: 13px 16px; border: none;
            color: #637c6c; background: transparent; font-weight: 500; }
        QPushButton[role="navigation"]:checked { color: #216d49; background: #eaf5ed; font-weight: 600; }
        QPushButton[role="navigation"]:hover { background: #f1f8f3; }
        QPushButton[role="key"] { background: #f5f9f6; font-size: 18px; padding: 8px; }
        QPushButton[role="key"]:hover { background: #eaf5ed; }
        QPushButton[role="operatorKey"] { background: #eaf5ed; color: #27784f; font-size: 18px; padding: 8px; }
        QPushButton[role="operatorKey"]:hover { background: #d9edde; }
        QPushButton[role="equals"] { font-size: 22px; padding: 8px; }
        QPushButton[role="sample"] { font-size: 11px; padding: 6px 10px; background: #f8fbf9; }
        QFrame[role="card"] { background: #ffffff; border: 1px solid #dfe9e2; border-radius: 14px; }
        QFrame[role="resultCard"] { background: #eaf5ed; border: 1px solid #d1e7d8; border-radius: 14px; }
        QLabel[role="resultCaption"] { color: #557b65; font-size: 12px; }
        QLabel[role="expressionResult"] { color: #195f3f; font-weight: 600; }
        QLabel[role="polynomialResult"] { color: #195f3f; font-size: 29px; font-weight: 600; }
        QLabel[role="resultContext"] { color: #64816e; font-size: 11px; }
        QLineEdit { background: #f8fbf9; border: 1px solid #dae6de; border-radius: 9px;
            padding: 11px 12px; color: #293e31; selection-background-color: #d8eddf; }
        QLineEdit:focus { border: 1px solid #66a581; background: #ffffff; }
        QLineEdit[invalid="true"] { border: 1px solid #d97078; background: #fff7f8; }
        QLineEdit[role="expressionInput"] { font-size: 21px; padding: 14px 12px; }
        QLineEdit[role="polynomialInput"] { font-size: 16px; }
        QLineEdit[role="resultSequence"] { border: none; background: #dcefe2; color: #316b49; font-size: 12px; }
        QLabel[role="preview"] { font-size: 21px; color: #3b5946; padding: 5px 0; }
        QLabel[role="error"] { color: #bc4f5e; font-size: 12px; }
        QLabel[role="success"] { color: #297956; font-size: 12px; }
        QComboBox { background: #f8fbf9; border: 1px solid #dae6de; border-radius: 9px;
            padding: 10px 14px; min-height: 18px; }
        QComboBox::drop-down { border: none; width: 24px; }
        QComboBox QAbstractItemView { background: white; border: 1px solid #dae6de;
            selection-background-color: #eaf5ed; padding: 5px; }
        QCheckBox { color: #607a69; spacing: 7px; font-size: 12px; }
        QCheckBox::indicator { width: 15px; height: 15px; border: 1px solid #bfcec4;
            background: white; border-radius: 4px; }
        QCheckBox::indicator:checked { background: #217a55; border: 1px solid #217a55;
            image: none; }
        QTableWidget { background: white; border: none; border-radius: 0px;
            gridline-color: #edf3ef; font-size: 11px; selection-background-color: #eaf5ed;
            selection-color: #216544; outline: 0; }
        QTableWidget::item { padding: 7px 8px; border-bottom: 1px solid #edf3ef; }
        QHeaderView::section { background: #f5f9f6; color: #6b8072; border: none;
            padding: 11px 8px; font-size: 11px; font-weight: 500; }
        QFrame[role="stackCard"] { background: #f5f9f6; border: 1px solid #e3ece6; border-radius: 10px; }
        QLabel[role="stackValue"] { color: #2f6d4a; font-size: 14px; font-family: "Menlo", "Consolas", monospace; }
        QScrollBar:vertical { background: transparent; width: 7px; margin: 0; }
        QScrollBar::handle:vertical { background: #cdded2; border-radius: 3px; min-height: 24px; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
        QScrollBar:horizontal { background: transparent; height: 7px; margin: 0; }
        QScrollBar::handle:horizontal { background: #cdded2; border-radius: 3px; min-width: 24px; }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
        QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: transparent; }
        QStatusBar { background: #ffffff; color: #758b7c; border-top: 1px solid #dfe8e2; font-size: 11px; }
        QStatusBar::item { border: none; }
        QSplitter::handle { background: transparent; }
        QFrame[role="asciiCard"] { background: #242c27; border: 1px solid #344b3d; border-radius: 14px; }
        QLabel[role="asciiHeading"] { color: #d9e9de; font-size: 15px; font-weight: 600; }
        QLabel[role="asciiCaption"] { color: #8dab98; font-size: 11px; }
        QPlainTextEdit[role="asciiArt"] { background: #242c27; color: #d5e8db; border: none;
            selection-background-color: #365c45; selection-color: #ffffff; }
    )QSS"));
}
