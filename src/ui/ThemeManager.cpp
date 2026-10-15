#include "ThemeManager.h"
#include <QApplication>
#include <QPalette>
namespace typatype {
void ThemeManager::apply(bool dark) {
    colors_=dark ? ThemeColors{"#241C35","#302540","#F8EFFF","#C8B7DF","#51405E","#423052","#CAA4F6","#C7A6F8","#FF9CB1","#573043"}
                 : ThemeColors{"#F7F0FF","#FFFAFF","#392553","#776289","#E9D7F4","#F0E3FF","#8553C2","#8054B1","#B43E57","#FAE0E7"};
    auto &c=colors_; auto n=[](const QColor &color){return color.name();};
    QPalette palette;
    palette.setColor(QPalette::Window,c.background);palette.setColor(QPalette::WindowText,c.text);
    palette.setColor(QPalette::Base,c.card);palette.setColor(QPalette::AlternateBase,c.soft);
    palette.setColor(QPalette::Text,c.text);palette.setColor(QPalette::Button,c.card);palette.setColor(QPalette::ButtonText,c.text);
    palette.setColor(QPalette::Highlight,c.primary);palette.setColor(QPalette::HighlightedText,dark?c.background:QColor("white"));
    palette.setColor(QPalette::ToolTipBase,c.card);palette.setColor(QPalette::ToolTipText,c.text);
    qApp->setPalette(palette);
    QString style=QString(R"(
      QWidget { color: %1; font-family: "DejaVu Sans", "Segoe UI", sans-serif; font-size: 14px; }
      QMainWindow, QWidget#shell { background: %2; }
      QLabel { background: transparent; }
      QLabel[role="muted"] { color: %3; }
      QLabel[role="heading"] { font-size: 28px; font-weight: 600; }
      QLabel[role="playHeading"] { font-size: 18px; font-weight: 600; }
      QLabel[role="section"] { font-size: 19px; font-weight: 600; }
      QLabel[role="metric"] { font-size: 48px; font-weight: 600; color: %6; }
      QFrame[role="card"] { background: %4; border: 1px solid %5; border-radius: 20px; }
      QPushButton { background: %4; border: 1px solid %5; border-radius: 12px; padding: 11px 18px; font-weight: 500; }
      QPushButton:hover { background: %7; border-color: %6; }
      QPushButton:focus { border: 2px solid %6; padding: 10px 17px; }
      QPushButton:pressed { background: %7; }
      QPushButton[role="primary"] { background: %6; color: %8; border-color: %6; font-weight: 600; }
      QPushButton[role="primary"]:hover { background: %9; border-color: %9; }
      QPushButton[role="nav"] { border: none; background: transparent; padding: 10px 20px; }
      QPushButton[role="nav"]:checked { background: %7; color: %6; }
      QPushButton[role="nav"]:focus { border: 2px solid %6; padding: 8px 18px; }
      QPushButton[role="quiet"] { border: none; background: transparent; color: %3; }
      QPushButton[role="choice"]:checked { background: %7; border-color: %6; color: %6; }
      QCheckBox { spacing: 12px; padding: 6px 0; }
      QCheckBox::indicator { width: 20px; height: 20px; border: 1px solid %5; border-radius: 6px; background: %4; }
      QCheckBox::indicator:checked { background: %6; border: 4px solid %7; }
      QSlider::groove:horizontal { height: 7px; background: %5; border-radius: 3px; }
      QSlider::sub-page:horizontal { background: %6; border-radius: 3px; }
      QSlider::handle:horizontal { width: 18px; margin: -6px 0; border-radius: 9px; background: %6; }
      QScrollArea { background: transparent; border: none; }
      QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }
      QScrollBar::handle:vertical { background: %5; min-height: 24px; border-radius: 4px; }
      QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
      QToolTip { background: %4; color: %1; border: 1px solid %5; padding: 8px; }
    )").arg(n(c.text),n(c.background),n(c.secondary),n(c.card),n(c.border),n(c.primary),n(c.soft),dark?n(c.background):"#FFFFFF",dark?"#D5B8FA":"#7643B4");
    qApp->setStyleSheet(style);
}
}
