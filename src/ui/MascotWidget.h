#pragma once
#include <QWidget>
#include <QSvgRenderer>
#include <QTimer>
#include <QVariantAnimation>
namespace typatype {
enum class MascotExpression { Welcome, Encouragement, Celebration, Smile, Wink, Curious };
class MascotWidget : public QWidget {
    Q_OBJECT
public:
    explicit MascotWidget(QWidget *parent=nullptr);
    void express(MascotExpression expression);
    MascotExpression expression() const {return expression_;}
    QSize sizeHint() const override {return {150,120};}
protected:
    void paintEvent(QPaintEvent*) override;
private:
    QSvgRenderer renderer_;
    QTimer reset_;
    QTimer idle_;
    int faceIndex_=0;
    QVariantAnimation gesture_;
    MascotExpression expression_=MascotExpression::Welcome;
    double gestureProgress_=0;
};
}
