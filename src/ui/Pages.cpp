#include "Pages.h"
#include <QButtonGroup>
#include <QApplication>
#include <QFontDatabase>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScrollArea>
#include <QScreen>
#include <QToolTip>
#include <QUuid>
#include <algorithm>
#include <cmath>
namespace typatype {
namespace {
QLabel* label(const QString &text,const char *role,QWidget *parent=nullptr) {
    auto *l=new QLabel(text,parent);l->setProperty("role",role);l->setWordWrap(true);return l;
}
QPushButton* button(const QString &text,const char *role,const char *name) {
    auto *b=new QPushButton(text);b->setProperty("role",role);b->setObjectName(name);b->setCursor(Qt::PointingHandCursor);return b;
}
QFrame* card() {
    auto *f=new QFrame;f->setProperty("role","card");
    auto *shadow=new QGraphicsDropShadowEffect(f);shadow->setBlurRadius(16);shadow->setOffset(0,3);shadow->setColor(QColor(57,37,83,12));f->setGraphicsEffect(shadow);return f;
}
QVBoxLayout* pageLayout(QWidget *page) {auto *l=new QVBoxLayout(page);l->setContentsMargins(0,0,0,0);l->setSpacing(18);return l;}
QFrame* metricCard(const QString &caption,QLabel *&value,const QString &initial="—") {
    auto *f=card();auto *l=new QVBoxLayout(f);l->setContentsMargins(24,20,24,20);l->setSpacing(5);
    l->addWidget(label(caption,"muted"));value=label(initial,"metric");l->addWidget(value);return f;
}
QScrollArea* scrollContents(QWidget *contents) {
    auto *scroll=new QScrollArea;scroll->setWidgetResizable(true);scroll->setWidget(contents);scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);return scroll;
}
}
SelectionDialog::SelectionDialog(const QString &title,const QStringList &options,int selected,QWidget *parent):QDialog(parent,Qt::Popup|Qt::FramelessWindowHint) {
    qApp->installEventFilter(this);
    setObjectName("selectionDialog");setWindowTitle(title);setAccessibleName(title);setModal(false);
    auto *outer=new QVBoxLayout(this);outer->setContentsMargins(2,2,2,2);
    auto *f=card();outer->addWidget(f);auto *layout=new QVBoxLayout(f);layout->setContentsMargins(20,18,20,20);layout->setSpacing(10);
    auto *head=new QHBoxLayout;head->addWidget(label(title,"section"));
    auto *close=button("×","quiet","closeSelector");close->setAccessibleName("Close selection dialog");close->setFixedWidth(40);head->addWidget(close);layout->addLayout(head);
    connect(close,&QPushButton::clicked,this,&QDialog::reject);
    auto *group=new QButtonGroup(this);group->setExclusive(true);
    for(int i=0;i<options.size();++i) {
        auto *b=button(options[i],"choice",qPrintable(QString("choice_%1").arg(i)));b->setCheckable(true);b->setChecked(i==selected);
        b->setMinimumHeight(48);b->setAutoDefault(false);group->addButton(b);layout->addWidget(b);choices_.push_back(b);
        connect(b,&QPushButton::clicked,this,[this,i]{selection_=i;accept();});
    }
    setFixedWidth(355);adjustSize();if(selected>=0&&selected<int(choices_.size())) choices_[selected]->setFocus();
}
bool SelectionDialog::eventFilter(QObject *object,QEvent *event) {
    if(isVisible() && event->type()==QEvent::MouseButtonPress) {
        auto *mouse=static_cast<QMouseEvent*>(event);
        if(!frameGeometry().contains(mouse->globalPosition().toPoint())) {
            outsideParent_=parentWidget()&&!parentWidget()->window()->frameGeometry().contains(mouse->globalPosition().toPoint());
            reject();return true;
        }
    }
    return QDialog::eventFilter(object,event);
}
void SelectionDialog::setCurrentChoice(int selected) {
    selection_=-1;outsideParent_=restoreParent_=false;
    for(int i=0;i<int(choices_.size());++i)choices_[i]->setChecked(i==selected);
    if(selected>=0&&selected<int(choices_.size()))choices_[selected]->setFocus(Qt::PopupFocusReason);
}
void SelectionDialog::done(int result) {
    // Restore the owning window when the interaction stays in this app. A click
    // into another app or dismissal while inactive must not steal its focus.
    restoreParent_=isVisible()&&!outsideParent_&&qApp->applicationState()==Qt::ApplicationActive;
    QDialog::done(result);
}
void SelectionDialog::keyPressEvent(QKeyEvent *e) {
    if(e->key()==Qt::Key_Down||e->key()==Qt::Key_Up) {
        int current=0;for(int i=0;i<int(choices_.size());++i)if(choices_[i]->hasFocus())current=i;
        current=(current+(e->key()==Qt::Key_Down?1:int(choices_.size())-1))%int(choices_.size());choices_[current]->setFocus();e->accept();
    } else if(e->key()==Qt::Key_Return||e->key()==Qt::Key_Enter) {
        for(auto *choice:choices_)if(choice->hasFocus()){choice->click();e->accept();return;}
        QDialog::keyPressEvent(e);
    } else QDialog::keyPressEvent(e);
}
PlayPage::PlayPage(ThemeManager &theme,SoundManager &sound,QWidget *parent):QWidget(parent),generator_(loadVocabulary()),sound_(sound) {
    setObjectName("playPage");auto *layout=pageLayout(this);
    layout->addWidget(label("Build your typing speed and accuracy.","playHeading"));
    layout->addWidget(label("Choose your difficulty and duration, then start typing to begin.","muted"));
    auto *controls=new QHBoxLayout;controls->setContentsMargins(0,4,0,2);controls->setSpacing(24);
    auto *difficultyColumn=new QVBoxLayout;difficultyColumn->setSpacing(5);difficultyColumn->addStretch();difficultyColumn->addWidget(label("Difficulty","muted"));
    difficulty_=button("Simple    ⌄","choice","difficultySelector");difficulty_->setMinimumWidth(250);difficulty_->setAccessibleName("Choose difficulty");difficultyColumn->addWidget(difficulty_);difficultyColumn->addStretch();
    auto *kittenColumn=new QVBoxLayout;kittenColumn->setSpacing(0);mascot_=new MascotWidget;mascot_->setObjectName("playMascot");cheer_=label("You’ve got this!","muted");cheer_->setObjectName("kittenMessage");cheer_->setAlignment(Qt::AlignCenter);cheer_->setFixedWidth(190);cheer_->setMinimumHeight(38);
    kittenColumn->addWidget(mascot_,0,Qt::AlignCenter);kittenColumn->addWidget(cheer_);
    auto *durationColumn=new QVBoxLayout;durationColumn->setSpacing(5);durationColumn->addStretch();durationColumn->addWidget(label("Duration","muted"));
    duration_=button("30 seconds    ⌄","choice","durationSelector");duration_->setMinimumWidth(190);duration_->setAccessibleName("Choose duration");durationColumn->addWidget(duration_);durationColumn->addStretch();
    controls->addStretch();controls->addLayout(difficultyColumn);controls->addLayout(kittenColumn);controls->addLayout(durationColumn);controls->addStretch();layout->addLayout(controls);
    board_=new TypingWidget(session_,theme);layout->addWidget(board_,1);
    auto *bottom=new QHBoxLayout;bottom->addWidget(label("Space: next word","muted"));bottom->addStretch();
    auto *restart=button("Restart","quiet","restartButton");restart->setIcon(QIcon(":/assets/restart.svg"));bottom->addWidget(restart);layout->addLayout(bottom);
    connect(restart,&QPushButton::clicked,this,&PlayPage::prepare);
    connect(difficulty_,&QPushButton::clicked,this,&PlayPage::chooseDifficulty);connect(duration_,&QPushButton::clicked,this,&PlayPage::chooseDuration);
    connect(board_,&TypingWidget::inputChanged,this,&PlayPage::refresh);connect(board_,&TypingWidget::characterInserted,&sound_,&SoundManager::type);
    connect(board_,&TypingWidget::wordAdvanced,&sound_,&SoundManager::advance);
    connect(board_,&TypingWidget::characterErased,&sound_,&SoundManager::erase);
    timer_.setInterval(25);timer_.setTimerType(Qt::PreciseTimer);connect(&timer_,&QTimer::timeout,this,&PlayPage::poll);
    cheerTimer_.setInterval(4500);cheerTimer_.setObjectName("kittenMessageTimer");
    connect(&cheerTimer_,&QTimer::timeout,this,[this]{
        if(!isVisible()||session_.state()==SessionState::Completed)return;
        const QStringList messages{"You’ve got this!","One word at a time.","Pawsitive progress!","Find your rhythm.","Keep those paws moving!","Small steps, big smiles."};
        cheerIndex_=(cheerIndex_+1)%messages.size();cheer_->setText(messages[cheerIndex_]);
    });
    session_.reset(generator_.generate(Difficulty::Simple,30),Difficulty::Simple,30);prepare();
}
void PlayPage::setOptions(Difficulty d,int duration) {
    timer_.stop();session_.reset(generator_.generate(d,duration),d,duration);delivered_=false;milestone_=0;
    difficulty_->setText(displayDifficulty(d)+"    ⌄");duration_->setText(QString::number(duration)+" seconds    ⌄");
    mascot_->express(MascotExpression::Welcome);cheerIndex_=0;cheer_->setText("You’ve got this!");cheerTimer_.start();board_->resetView();board_->setFocus(Qt::OtherFocusReason);
}
void PlayPage::prepare() {setOptions(session_.difficulty(),session_.duration());}
void PlayPage::abandon() {timer_.stop();prepare();}
void PlayPage::applyPreferences(const Preferences &p) {board_->setTextSize(p.textSize);update();}
void PlayPage::restoreTypingFocus() {
    if(!isVisible()||QApplication::activePopupWidget()||QApplication::activeModalWidget())return;
    board_->setFocus(Qt::OtherFocusReason);
    // Popup teardown/window activation may finish in the next event-loop turn.
    QTimer::singleShot(0,board_,[this]{
        if(isVisible()&&!QApplication::activePopupWidget()&&!QApplication::activeModalWidget())
            board_->setFocus(Qt::OtherFocusReason);
    });
}
void PlayPage::chooseDifficulty() {
    showSelector(difficultyDialog_,"Choose difficulty",{"Simple","Punctuation","Punctuation + Numbers"},int(session_.difficulty()),
        [this](int choice){setOptions(static_cast<Difficulty>(choice),session_.duration());});
}
void PlayPage::chooseDuration() {
    const QList<int> durations{15,30,60};
    showSelector(durationDialog_,"Choose duration",{"15 seconds","30 seconds","60 seconds"},durations.indexOf(session_.duration()),
        [this,durations](int choice){setOptions(session_.difficulty(),durations[choice]);});
}
void PlayPage::showSelector(SelectionDialog *&dialog,const QString &title,const QStringList &options,int selected,std::function<void(int)> apply) {
    if(!dialog) {
        dialog=new SelectionDialog(title,options,selected,this);
        connect(this,&PlayPage::roundCompleted,dialog,&QDialog::reject);
        connect(dialog,&QDialog::accepted,this,[dialog,apply=std::move(apply)]{apply(dialog->selection());});
        connect(dialog,&QDialog::finished,this,[this,dialog]{
            const bool activate=dialog->shouldRestoreParentActivation();
            QTimer::singleShot(0,board_,[this,activate]{
                if(QApplication::activePopupWidget()||QApplication::activeModalWidget())return;
                if(activate&&window()->isVisible())window()->activateWindow();
                restoreTypingFocus();
            });
        });
    }
    auto *anchor=(&dialog==&difficultyDialog_)?difficulty_:duration_;
    const auto origin=anchor->mapToGlobal(QPoint(0,0));
    const auto available=anchor->screen()->availableGeometry();
    int x=std::clamp(origin.x(),available.left(),std::max(available.left(),available.right()-dialog->width()+1));
    int y=origin.y()+anchor->height()+4;
    if(y+dialog->height()>available.bottom()+1)y=origin.y()-dialog->height()-4;
    y=std::clamp(y,available.top(),std::max(available.top(),available.bottom()-dialog->height()+1));
    dialog->move(x,y);
    // A modeless popup grabs selection input without nesting a modal event loop
    // or disrupting the parent window's focus/round timer.
    dialog->show();
    dialog->setCurrentChoice(selected);
}
void PlayPage::poll() {session_.tick();refresh();}
void PlayPage::refresh() {
    board_->update();
    if(session_.state()==SessionState::Running && !timer_.isActive()) timer_.start();
    auto milestone=session_.lockedWords()/10;
    if(session_.state()==SessionState::Running && milestone>milestone_) {
        milestone_=milestone;mascot_->express(MascotExpression::Encouragement);cheer_->setText(milestone%2?"Nice work!":"Keep going!");
        cheerTimer_.start();
        QTimer::singleShot(0,&sound_,&SoundManager::encourage);
    }
    if(session_.state()==SessionState::Completed&&!delivered_) {
        delivered_=true;timer_.stop();cheerTimer_.stop();
        RoundResult r{QUuid::createUuid().toString(QUuid::WithoutBraces),QDateTime::currentDateTimeUtc(),session_.difficulty(),session_.duration(),int(session_.words().size()),session_.score()};
        emit roundCompleted(r);
        QTimer::singleShot(0,&sound_,&SoundManager::celebrate);
    }
}
ResultsPage::ResultsPage(QWidget *parent):QWidget(parent) {
    setObjectName("resultsPage");auto *layout=pageLayout(this);layout->addStretch();
    mascot_=new MascotWidget;mascot_->setObjectName("resultMascot");mascot_->setFixedSize(172,145);layout->addWidget(mascot_,0,Qt::AlignCenter);
    message_=label("Nice work!","heading");message_->setAlignment(Qt::AlignCenter);layout->addWidget(message_);
    context_=label({},"muted");context_->setAlignment(Qt::AlignCenter);layout->addWidget(context_);
    auto *metrics=new QHBoxLayout;metrics->setSpacing(20);metrics->addWidget(metricCard("WORDS PER MINUTE",wpm_));metrics->addWidget(metricCard("ACCURACY",accuracy_));layout->addLayout(metrics);
    best_=label({},"section");best_->setAlignment(Qt::AlignCenter);layout->addWidget(best_);
    comparison_=label({},"muted");comparison_->setAlignment(Qt::AlignCenter);layout->addWidget(comparison_);
    saved_=label({},"muted");saved_->setAlignment(Qt::AlignCenter);layout->addWidget(saved_);
    auto *actions=new QHBoxLayout;actions->addStretch();auto *again=button("Play Again","primary","playAgainButton");auto *progress=button("View Progress","choice","viewProgressButton");
    actions->addWidget(again);actions->addWidget(progress);actions->addStretch();layout->addLayout(actions);layout->addStretch();
    connect(again,&QPushButton::clicked,this,&ResultsPage::playAgain);connect(progress,&QPushButton::clicked,this,&ResultsPage::viewProgress);
}
void ResultsPage::showResult(const RoundResult &r,const RoundResult *previous,bool bestWpm,bool bestAccuracy,bool saved) {
    wpm_->setText(QString::number(r.score.wpm,'f',1));accuracy_->setText(QString::number(r.score.accuracy,'f',1)+"%");
    context_->setText(QString("%1 · %2 seconds · %3 words").arg(displayDifficulty(r.difficulty)).arg(r.duration).arg(r.wordCount));
    message_->setText(bestWpm||bestAccuracy?"A new personal best!":"Nice work!");mascot_->express(MascotExpression::Celebration);
    best_->setText(bestWpm&&bestAccuracy?"Personal best · WPM & accuracy":bestWpm?"Personal best · WPM":bestAccuracy?"Personal best · accuracy":"A little practice makes progress.");
    comparison_->setText(comparisonText(r,previous));saved_->setText(saved?"Saved on this device":"This round was not saved. See the message above for details.");
}
ProgressChart::ProgressChart(ThemeManager &theme,QWidget *parent):QWidget(parent),theme_(theme) {
    setObjectName("progressChart");setMouseTracking(true);setFocusPolicy(Qt::StrongFocus);setMinimumHeight(240);setAccessibleName("Completed round progress chart");
}
void ProgressChart::setResults(const std::vector<RoundResult> &rounds) {
    rounds_=rounds;speedBests_.clear();accuracyBests_.clear();
    double speed=-1,accuracy=-1;
    for(const auto &round:rounds_) {
        speedBests_.push_back(round.score.wpm>speed);accuracyBests_.push_back(round.score.accuracy>accuracy);
        speed=std::max(speed,round.score.wpm);accuracy=std::max(accuracy,round.score.accuracy);
    }
    changeRange(rounds_.size()>20?rounds_.size()-20:0);
}
void ProgressChart::changeRange(std::size_t first) {
    first_=first;selected_=hovered_=-1;points_.clear();
    emit visibleRangeChanged();emit pointDetailsChanged({});update();
}
void ProgressChart::showOlder() {changeRange(first_>20?first_-20:0);}
void ProgressChart::showNewer() {
    const auto last=rounds_.size()>20?rounds_.size()-20:0;
    changeRange(std::min(last,first_+20));
}
void ProgressChart::setAccuracy(bool accuracy) {accuracy_=accuracy;hovered_=-1;points_.clear();update();}
void ProgressChart::paintEvent(QPaintEvent*) {
    QPainter p(this);p.setRenderHint(QPainter::Antialiasing);auto &c=theme_.colors();
    p.setPen(QPen(c.border));p.setBrush(c.card);p.drawRoundedRect(rect().adjusted(1,1,-1,-1),18,18);
    QFont font=p.font();font.setPixelSize(12);p.setFont(font);p.setPen(c.secondary);points_.clear();
    if(rounds_.empty()) {p.drawText(rect().adjusted(25,25,-25,-25),Qt::AlignCenter|Qt::TextWordWrap,"Complete your first round to see your progress here.");return;}
    const auto first=first_,count=visibleCount();
    const QRectF plot(65,25,width()-90,height()-88);
    auto metric=[this](const RoundResult &r){return accuracy_?r.score.accuracy:r.score.wpm;};
    double maximum=0;
    for(std::size_t i=first;i<first+count;++i)maximum=std::max(maximum,metric(rounds_[i]));
    const double scale=accuracy_?100.0:std::max(20.0,std::ceil(maximum/20.0)*20.0);
    for(int tick=0;tick<=4;++tick) {
        double y=plot.bottom()-tick*plot.height()/4.0;p.setPen(QPen(c.border,1,Qt::DotLine));p.drawLine(QPointF(plot.left(),y),QPointF(plot.right(),y));
        p.setPen(c.secondary);p.drawText(QRectF(8,y-10,47,20),Qt::AlignRight|Qt::AlignVCenter,QString::number(tick*scale/4.0,'f',0)+(accuracy_?"%":""));
    }
    p.setPen(c.secondary);p.drawText(QRectF(plot.left(),height()-27,plot.width(),20),Qt::AlignCenter,"Completed rounds");
    QPainterPath path;
    for(std::size_t i=0;i<count;++i) {
        QPointF point(plot.left()+(count==1?plot.width()/2.0:i*plot.width()/(count-1)),plot.bottom()-metric(rounds_[first+i])/scale*plot.height());
        points_.push_back(point);if(!i)path.moveTo(point);else path.lineTo(point);
        if(count<=10 || i%4==0 || i==count-1) {p.setPen(c.secondary);p.drawText(QRectF(point.x()-22,plot.bottom()+8,44,20),Qt::AlignCenter,QString::number(first+i+1));}
    }
    p.setPen(QPen(c.primary,2.5));p.setBrush(Qt::NoBrush);p.drawPath(path);
    for(std::size_t i=0;i<points_.size();++i) {
        bool best=accuracy_?accuracyBests_[first+i]:speedBests_[first+i];
        p.setBrush(c.primary);p.setPen(QPen(c.card,2));p.drawEllipse(points_[i],(hovered_==int(i)||selected_==int(i))?6.5:4.5,(hovered_==int(i)||selected_==int(i))?6.5:4.5);
        if(best) {p.setPen(QPen(c.primary,1));p.setBrush(Qt::NoBrush);p.drawEllipse(points_[i],9,9);}
    }
}
int ProgressChart::nearest(QPointF point) const {
    int best=-1;double distance=20;
    for(int i=0;i<int(points_.size());++i) {auto delta=point-points_[i];double d=std::hypot(delta.x(),delta.y());if(d<distance){distance=d;best=i;}}
    return best;
}
QString ProgressChart::pointDetails(int index) const {
    if(index<0 || index>=int(visibleCount()))return {};
    const auto &r=rounds_[first_+index];
    return QString("%1 · %2 WPM · %3% accuracy\n%4 · %5 seconds · %6 words")
        .arg(r.completedAt.toLocalTime().toString("dd MMM yyyy, h:mm:ss AP"),QString::number(r.score.wpm,'f',1),QString::number(r.score.accuracy,'f',1),displayDifficulty(r.difficulty)).arg(r.duration).arg(r.wordCount);
}
void ProgressChart::showPoint(int index,QPoint global) {
    const auto details=pointDetails(index);
    if(details.isEmpty()) {QToolTip::hideText();return;}
    emit pointDetailsChanged(details);QToolTip::showText(global,details,this);
}
void ProgressChart::mouseMoveEvent(QMouseEvent *e) {
    hovered_=nearest(e->position());
    if(hovered_>=0)showPoint(hovered_,e->globalPosition().toPoint());
    else {QToolTip::hideText();emit pointDetailsChanged(pointDetails(selected_));}
    update();
}
void ProgressChart::mousePressEvent(QMouseEvent *e) {
    setFocus(Qt::MouseFocusReason);selected_=nearest(e->position());
    showPoint(selected_,e->globalPosition().toPoint());emit pointDetailsChanged(pointDetails(selected_));update();
}
void ProgressChart::leaveEvent(QEvent *e) {
    hovered_=-1;QToolTip::hideText();emit pointDetailsChanged(pointDetails(selected_));update();QWidget::leaveEvent(e);
}
void ProgressChart::keyPressEvent(QKeyEvent *e) {
    const int count=int(visibleCount());
    if(!count){QWidget::keyPressEvent(e);return;}
    switch(e->key()) {
    case Qt::Key_Right: selected_=selected_<0?0:std::min(count-1,selected_+1);break;
    case Qt::Key_Left: selected_=selected_<0?count-1:std::max(0,selected_-1);break;
    case Qt::Key_Home: selected_=0;break;
    case Qt::Key_End: selected_=count-1;break;
    default: QWidget::keyPressEvent(e);return;
    }
    hovered_=-1;emit pointDetailsChanged(pointDetails(selected_));update();e->accept();
}
ProgressPage::ProgressPage(ThemeManager &theme,QWidget *parent):QWidget(parent) {
    setObjectName("progressPage");auto *outer=pageLayout(this);auto *content=new QWidget;auto *layout=pageLayout(content);
    layout->addWidget(label("Your little steps add up.","heading"));layout->addWidget(label("Every completed round, all in one place.","muted"));
    auto *metrics=new QHBoxLayout;metrics->setSpacing(14);
    auto *w=metricCard("BEST WPM",bestWpm_);auto *a=metricCard("BEST ACCURACY",bestAccuracy_);auto *n=metricCard("COMPLETED ROUNDS",count_,"0");
    bestWpm_->setProperty("role","section");bestAccuracy_->setProperty("role","section");
    metrics->addWidget(w,1);metrics->addWidget(a,1);metrics->addWidget(n,1);layout->addLayout(metrics);
    comparison_=label("Your next round starts a little story.","muted");layout->addWidget(comparison_);
    auto *chartHeader=new QHBoxLayout;chartHeader->addWidget(label("Progress over time","section"));chartHeader->addStretch();
    auto *group=new QButtonGroup(this);group->setExclusive(true);auto *wpm=button("WPM","choice","chartWpmButton");auto *acc=button("Accuracy","choice","chartAccuracyButton");
    wpm->setCheckable(true);acc->setCheckable(true);wpm->setChecked(true);group->addButton(wpm);group->addButton(acc);chartHeader->addWidget(wpm);chartHeader->addWidget(acc);layout->addLayout(chartHeader);
    auto *rangeRow=new QHBoxLayout;range_=label({},"muted");range_->setObjectName("chartRange");rangeRow->addWidget(range_);rangeRow->addStretch();
    older_=button("← Older","quiet","chartOlderButton");newer_=button("Newer →","quiet","chartNewerButton");rangeRow->addWidget(older_);rangeRow->addWidget(newer_);layout->addLayout(rangeRow);
    chart_=new ProgressChart(theme);
    pointDetails_=label({},"muted");pointDetails_->setObjectName("chartPointDetails");pointDetails_->setMinimumHeight(42);layout->addWidget(pointDetails_);
    layout->addWidget(chart_);
    layout->addWidget(label("Hover or click a point, or use arrow keys on the chart. Rings mark new personal bests.","muted"));
    connect(chart_,&ProgressChart::visibleRangeChanged,this,&ProgressPage::updateRange);
    connect(chart_,&ProgressChart::pointDetailsChanged,this,[this](const QString &details){pointDetails_->setText(details.isEmpty()?"Select a round to see its details.":details);});
    connect(older_,&QPushButton::clicked,chart_,&ProgressChart::showOlder);connect(newer_,&QPushButton::clicked,chart_,&ProgressChart::showNewer);
    connect(wpm,&QPushButton::clicked,this,[this]{chart_->setAccuracy(false);});connect(acc,&QPushButton::clicked,this,[this]{chart_->setAccuracy(true);});
    layout->addWidget(label("Recent results","section"));recent_=new QVBoxLayout;recent_->setSpacing(10);layout->addLayout(recent_);layout->addStretch();outer->addWidget(scrollContents(content));
}
void ProgressPage::updateRange() {
    const auto start=chart_->visibleStart(),count=chart_->visibleCount(),total=chart_->totalCount();
    range_->setText(count?QString("Rounds %1–%2 of %3").arg(start+1).arg(start+count).arg(total):"No completed rounds yet");
    older_->setEnabled(start>0);newer_->setEnabled(start+count<total);
}
void ProgressPage::refresh(const std::vector<RoundResult> &rounds) {
    chart_->setResults(rounds);count_->setText(QString::number(rounds.size()));
    while(auto *item=recent_->takeAt(0)) {delete item->widget();delete item;}
    if(rounds.empty()) {bestWpm_->setText("—");bestAccuracy_->setText("—");comparison_->setText("Your next round starts a little story.");recent_->addWidget(label("Complete your first round to see your progress here.","muted"));return;}
    const auto &speed=*std::max_element(rounds.begin(),rounds.end(),[](const auto &a,const auto &b){return a.score.wpm<b.score.wpm;});
    const auto &accuracy=*std::max_element(rounds.begin(),rounds.end(),[](const auto &a,const auto &b){return a.score.accuracy<b.score.accuracy;});
    bestWpm_->setText(QString("%1 WPM\n%2% accuracy\n%3 · %4s").arg(QString::number(speed.score.wpm,'f',1),QString::number(speed.score.accuracy,'f',1),displayDifficulty(speed.difficulty)).arg(speed.duration));
    bestAccuracy_->setText(QString("%1%\n%2 WPM\n%3 · %4s").arg(QString::number(accuracy.score.accuracy,'f',1),QString::number(accuracy.score.wpm,'f',1),displayDifficulty(accuracy.difficulty)).arg(accuracy.duration));
    comparison_->setText(comparisonText(rounds.back(),rounds.size()>1?&rounds[rounds.size()-2]:nullptr));
    const auto first=rounds.size()>20?rounds.size()-20:0;
    for(std::size_t i=rounds.size();i>first;--i) {
        const auto &r=rounds[i-1];auto *f=card();auto *row=new QHBoxLayout(f);row->setContentsMargins(18,14,18,14);row->setSpacing(16);
        auto *date=label(r.completedAt.toLocalTime().toString("dd MMM yyyy\nh:mm:ss AP"),"muted");row->addWidget(date,1);
        row->addWidget(label(QString("%1 WPM\n%2% accuracy").arg(QString::number(r.score.wpm,'f',1),QString::number(r.score.accuracy,'f',1)),"section"),1);
        auto *context=label(QString("%1\n%2 seconds · %3 words").arg(displayDifficulty(r.difficulty)).arg(r.duration).arg(r.wordCount),"muted");row->addWidget(context,1);recent_->addWidget(f);
    }
}
SettingsPage::SettingsPage(const Preferences &preferences,ThemeManager &theme,QWidget *parent):QWidget(parent),theme_(theme) {
    setObjectName("settingsPage");auto *outer=pageLayout(this);auto *content=new QWidget;auto *layout=pageLayout(content);
    layout->addWidget(label("Make yourself comfortable.","heading"));layout->addWidget(label("Your preferences are saved on this device.","muted"));
    auto *appearance=card();auto *a=new QVBoxLayout(appearance);a->setContentsMargins(26,22,26,24);a->setSpacing(14);a->addWidget(label("Appearance","section"));
    dark_=new QCheckBox("Dark theme");dark_->setObjectName("darkThemeCheck");dark_->setChecked(preferences.dark);a->addWidget(dark_);
    auto *sizeRow=new QHBoxLayout;sizeRow->addWidget(label("Typing text size","muted"));sizeRow->addStretch();sizeValue_=label({},"section");sizeRow->addWidget(sizeValue_);a->addLayout(sizeRow);
    size_=new QSlider(Qt::Horizontal);size_->setObjectName("textSizeSlider");size_->setRange(22,40);size_->setValue(preferences.textSize);size_->setAccessibleName("Typing text size in pixels");a->addWidget(size_);
    preview_=label("gentle cloud smile","section");preview_->setObjectName("textSizePreview");preview_->setMinimumHeight(70);preview_->setAlignment(Qt::AlignCenter);a->addWidget(preview_);layout->addWidget(appearance);
    auto *sound=card();auto *s=new QVBoxLayout(sound);s->setContentsMargins(26,22,26,24);s->setSpacing(14);s->addWidget(label("Sound","section"));
    typing_=new QCheckBox("Click, typing, word, and Backspace sounds");typing_->setObjectName("typingSoundCheck");typing_->setChecked(preferences.typingSound);s->addWidget(typing_);
    celebration_=new QCheckBox("Encouragement and celebration sounds");celebration_->setObjectName("celebrationSoundCheck");celebration_->setChecked(preferences.celebrationSound);s->addWidget(celebration_);
    auto *volumeRow=new QHBoxLayout;volumeRow->addWidget(label("Volume","muted"));volumeRow->addStretch();volumeValue_=label({},"section");volumeRow->addWidget(volumeValue_);s->addLayout(volumeRow);
    volume_=new QSlider(Qt::Horizontal);volume_->setObjectName("volumeSlider");volume_->setRange(0,100);volume_->setValue(preferences.volume);volume_->setAccessibleName("Sound volume percent");s->addWidget(volume_);
    testSound_=new QPushButton("Test sound");testSound_->setObjectName("testSoundButton");testSound_->setProperty("role","soft");s->addWidget(testSound_,0,Qt::AlignLeft);
    connect(testSound_,&QPushButton::clicked,this,&SettingsPage::soundPreviewRequested);layout->addWidget(sound);
    layout->addWidget(label("Space advances to the next word (Enter also works). Correct submitted words stay locked.\nWrong letters turn red without replacing the target. Backspace returns erased letters to grey.\nWPM uses final matching characters; accuracy remembers every character attempt.","muted"));layout->addStretch();outer->addWidget(scrollContents(content));
    auto changed=[this]{emit preferencesChanged();refreshPreview();};
    connect(dark_,&QCheckBox::toggled,this,changed);connect(typing_,&QCheckBox::toggled,this,changed);connect(celebration_,&QCheckBox::toggled,this,changed);
    connect(size_,&QSlider::valueChanged,this,changed);connect(volume_,&QSlider::valueChanged,this,changed);refreshPreview();
}
Preferences SettingsPage::preferences() const {return {dark_->isChecked(),size_->value(),typing_->isChecked(),celebration_->isChecked(),volume_->value()};}
void SettingsPage::refreshPreview() {
    sizeValue_->setText(QString::number(size_->value())+" px");volumeValue_->setText(QString::number(volume_->value())+"%");
    testSound_->setEnabled(volume_->value()>0&&(typing_->isChecked()||celebration_->isChecked()));
    const auto mono=QFontDatabase::systemFont(QFontDatabase::FixedFont);
    preview_->setStyleSheet(QString("font-family: '%1'; font-size: %2px; color: %3;").arg(mono.family()).arg(size_->value()).arg(theme_.colors().primary.name()));
}
}
