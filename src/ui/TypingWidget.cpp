#include "TypingWidget.h"
#include "storage/Stores.h"
#include <QFontDatabase>
#include <QGraphicsDropShadowEffect>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <algorithm>
#include <cmath>
namespace typatype {
TypingWidget::TypingWidget(GameSession &session,ThemeManager &theme,QWidget *parent):QWidget(parent),session_(session),theme_(theme) {
    auto *shadow=new QGraphicsDropShadowEffect(this);shadow->setBlurRadius(20);shadow->setOffset(0,4);shadow->setColor(QColor(57,37,83,15));setGraphicsEffect(shadow);
    setObjectName("typingBoard"); setFocusPolicy(Qt::StrongFocus); setMinimumSize(360,240);
    setAccessibleName("Typing board"); setAccessibleDescription("Type the displayed words. Space advances. Correct submitted words are locked.");
    setToolTip("Type here. Space moves to the next word. Correct submitted words are locked.");
    blink_.setInterval(520); connect(&blink_,&QTimer::timeout,this,[this]{caretOn_=!caretOn_;if(hasFocus()) update();});blink_.start();
}
void TypingWidget::keyPressEvent(QKeyEvent *event) {
    if(event->modifiers() & (Qt::ControlModifier|Qt::AltModifier|Qt::MetaModifier)) {QWidget::keyPressEvent(event);return;}
    // Expire before handling input, including events between the repaint timer ticks.
    bool changed=session_.tick();
    bool inserted=false,advanced=false,erased=false;
    if(event->key()==Qt::Key_Space||event->key()==Qt::Key_Return||event->key()==Qt::Key_Enter) {
        advanced=session_.enter();changed=advanced||changed;
    } else if(event->key()==Qt::Key_Backspace) {
        erased=session_.backspace();changed=erased||changed;
    }
    else {
        for(char32_t c:event->text().toUcs4()) {
            if(c<32 || c==127 || (c>=128 && !QChar::isPrint(c))) continue;
            if(session_.insert(c)) {changed=true;inserted=true;}
        }
    }
    if(changed) {caretOn_=true;blink_.start();emit inputChanged();update();}
    // Start/update gameplay before optional audio initializes its device.
    if(inserted && session_.state()!=SessionState::Completed)emit characterInserted();
    if(advanced)emit wordAdvanced();
    if(erased)emit characterErased();
    event->accept();
}
void TypingWidget::mousePressEvent(QMouseEvent *e) {setFocus(Qt::MouseFocusReason);QWidget::mousePressEvent(e);}
void TypingWidget::setTextSize(int size) {
    textSize_=size;scroll_=0;
    QFont mono=QFontDatabase::systemFont(QFontDatabase::FixedFont);mono.setPixelSize(size);
    // Keep room for both the active line and one complete upcoming line.
    setMinimumHeight(std::max(240,152+int(std::ceil(2*QFontMetricsF(mono).height()+18))+4));update();
}
void TypingWidget::focusInEvent(QFocusEvent *e) {caretOn_=true;update();QWidget::focusInEvent(e);}
void TypingWidget::focusOutEvent(QFocusEvent *e) {update();QWidget::focusOutEvent(e);}
void TypingWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);p.setRenderHint(QPainter::Antialiasing); auto &c=theme_.colors();
    p.setBrush(c.card);p.setPen(QPen(hasFocus()?c.primary:c.border,hasFocus()?1.5:1));p.drawRoundedRect(rect().adjusted(1,1,-1,-1),22,22);
    QFont label=p.font();label.setPixelSize(13);p.setFont(label);p.setPen(c.secondary);
    p.drawText(QRect(30,23,width()-150,23),Qt::AlignLeft|Qt::AlignVCenter,"English · "+displayDifficulty(session_.difficulty()));
    QFont clock=label;clock.setPixelSize(20);clock.setBold(true);p.setFont(clock);p.setPen(c.primary);
    p.drawText(QRect(width()-100,18,70,30),Qt::AlignRight|Qt::AlignVCenter,QString::number(int(std::ceil(session_.remaining())))+"s");
    p.setPen(QPen(c.border));p.drawLine(30,63,width()-30,63);
    QFont mono=QFontDatabase::systemFont(QFontDatabase::FixedFont);mono.setPixelSize(textSize_);p.setFont(mono);
    const QFontMetricsF fm(mono);const double cell=fm.horizontalAdvance('m'),line=fm.height()+18;
    const double left=30,right=width()-30,top=87,bottom=height()-65;
    struct Glyph {QPointF point;char32_t ch;bool correct=false,error=false,missing=false;};
    std::vector<Glyph> glyphs; double x=left,y=top;QPointF caret(left,top);
    const auto &words=session_.words();
    for(std::size_t wi=0;wi<words.size();++wi) {
        const auto &word=words[wi];auto length=std::max(word.target.size(),word.input.size());
        // Move an ordinary token as a whole; split exceptionally long extra input safely.
        if(x>left && x+length*cell>right) {x=left;y+=line;}
        for(std::size_t i=0;i<length;++i) {
            if(x+cell>right) {x=left;y+=line;}
            if(wi==session_.activeIndex() && i==word.input.size()) caret={x,y};
            Glyph g;g.point={x,y};
            if(i<word.input.size()) {
                g.correct=i<word.target.size()&&word.input[i]==static_cast<unsigned char>(word.target[i]);g.error=!g.correct;
                // Keep the expected letter visible; the actual attempt stays in
                // the session for correction and accurate scoring.
                g.ch=i<word.target.size()?static_cast<unsigned char>(word.target[i]):word.input[i];
            } else {g.ch=word.target[i];g.missing=word.submitted;}
            glyphs.push_back(g);x+=cell;
        }
        if(wi==session_.activeIndex() && word.input.size()==length) {
            if(x+cell>right) {x=left;y+=line;}
            caret={x,y};
        }
        x+=cell*1.05;
    }
    if(session_.activeIndex()>=words.size()) {if(x+cell>right){x=left;y+=line;}caret={x,y};}
    const double viewport=std::max(50.0,bottom-top);
    const bool hasNextLine=!glyphs.empty()&&glyphs.back().point.y()>caret.y();
    const double visibleBottom=bottom-(hasNextLine?line+2:0);
    if(caret.y()-scroll_+fm.height()>visibleBottom) scroll_=caret.y()+fm.height()-visibleBottom;
    if(caret.y()-scroll_<top) scroll_=caret.y()-top;
    scroll_=std::clamp(scroll_,0.0,std::max(0.0,y+line-top-viewport));
    p.save();p.setClipRect(QRectF(left-4,top-6,right-left+8,viewport+8));
    for(auto &g:glyphs) {
        auto point=g.point-QPointF(0,scroll_);
        if(point.y()<top-1 || point.y()+fm.height()>bottom+0.01) continue;
        if(g.error||g.missing) {p.setPen(Qt::NoPen);p.setBrush(c.errorSurface);p.drawRoundedRect(QRectF(point.x()-1,point.y(),cell+1,fm.height()+3),3,3);}
        p.setPen(g.error||g.missing?c.error:g.correct?c.correct:c.secondary);
        const auto character=QString::fromUcs4(&g.ch,1);
        p.drawText(point+QPointF(0,fm.ascent()),g.ch==U' '&&g.error?QString("·"):character);
        if(g.missing) {p.setPen(QPen(c.error,1.5,Qt::DotLine));p.drawLine(point+QPointF(0,fm.height()+1),point+QPointF(cell-2,fm.height()+1));}
    }
    if(caretOn_&&hasFocus()&&session_.state()!=SessionState::Completed) {
        p.setPen(QPen(c.primary,2));auto point=caret-QPointF(0,scroll_);p.drawLine(point-QPointF(1,1),point+QPointF(-1,fm.height()+2));
    }
    p.restore();p.setFont(label);p.setPen(c.secondary);
    QString hint=session_.state()==SessionState::Ready?"Start typing to begin":session_.activeIndex()>=words.size()?"All words submitted · waiting for the timer":hasFocus()?"A little focus, one word at a time.":"Click here to continue typing · timer is still running";
    p.drawText(QRect(30,height()-42,width()-60,24),Qt::AlignLeft|Qt::AlignVCenter,hint);
    // Accessible content mirrors the real targets; painting remains independent of input.
    QStringList targets;for(const auto &w:words) targets<<QString::fromStdString(w.target);
    setAccessibleDescription(targets.join(' ')+". "+hint);
}
}
