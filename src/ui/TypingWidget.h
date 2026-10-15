#pragma once
#include "core/GameSession.h"
#include "ThemeManager.h"
#include <QTimer>
#include <QWidget>
namespace typatype {
class TypingWidget : public QWidget {
    Q_OBJECT
public:
    TypingWidget(GameSession &session,ThemeManager &theme,QWidget *parent=nullptr);
    void setTextSize(int size);
    void resetView() {scroll_=0;caretOn_=true;update();}
    QSize sizeHint() const override {return {900,340};}
signals:
    void inputChanged();
    void characterInserted();
    void wordAdvanced();
    void characterErased();
protected:
    void paintEvent(QPaintEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void focusInEvent(QFocusEvent*) override;
    void focusOutEvent(QFocusEvent*) override;
private:
    GameSession &session_;
    ThemeManager &theme_;
    int textSize_=30;
    double scroll_=0;
    bool caretOn_=true;
    QTimer blink_;
};
}
