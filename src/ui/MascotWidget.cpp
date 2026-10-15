#include "MascotWidget.h"
#include <QPainter>
#include <cmath>
namespace typatype {
MascotWidget::MascotWidget(QWidget *parent):QWidget(parent) {
    setFixedSize(146,116); setAccessibleName("White kitten face and front paws with a violet bow");
    reset_.setSingleShot(true);
    idle_.setInterval(4500);idle_.setObjectName("kittenFaceTimer");
    connect(&idle_,&QTimer::timeout,this,[this]{
        if(!isVisible())return;
        constexpr MascotExpression faces[]{MascotExpression::Smile,MascotExpression::Wink,MascotExpression::Curious};
        const auto face=faces[faceIndex_];faceIndex_=(faceIndex_+1)%3;express(face);
    });
    gesture_.setStartValue(0.0);gesture_.setEndValue(1.0);
    connect(&gesture_,&QVariantAnimation::valueChanged,this,[this](const QVariant &value){gestureProgress_=value.toDouble();update();});
    connect(&reset_,&QTimer::timeout,this,[this]{express(MascotExpression::Welcome);});
    express(MascotExpression::Welcome);
}
void MascotWidget::express(MascotExpression e) {
    reset_.stop();idle_.stop();gesture_.stop();gestureProgress_=0;expression_=e;
    QString name;
    switch(e) {
        case MascotExpression::Welcome:name="welcome";break;
        case MascotExpression::Encouragement:name="encouragement";break;
        case MascotExpression::Celebration:name="celebration";break;
        case MascotExpression::Smile:name="smile";break;
        case MascotExpression::Wink:name="wink";break;
        case MascotExpression::Curious:name="curious";break;
    }
    renderer_.load(":/assets/kitten-"+name+".svg"); update();
    if(e==MascotExpression::Welcome)idle_.start();
    else if(e!=MascotExpression::Celebration)reset_.start(1400);
    if(e!=MascotExpression::Welcome) {gesture_.setDuration(e==MascotExpression::Celebration?700:450);gesture_.start();}
}
void MascotWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
    constexpr double pi=3.141592653589793;
    p.translate(width()/2.0,height()/2.0-4.0*std::sin(pi*gestureProgress_));
    p.rotate(1.5*std::sin(2*pi*gestureProgress_));p.translate(-width()/2.0,-height()/2.0);
    renderer_.render(&p,rect().adjusted(4,4,-4,-4));
}
}
