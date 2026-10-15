#include "MainWindow.h"
#include <QApplication>
#include <QButtonGroup>
#include <QCloseEvent>
#include <QDir>
#include <QIcon>
#include <QScreen>
#include <QUuid>
#include <algorithm>
void initializeTypaTypeResources() {Q_INIT_RESOURCE(resources);}
namespace typatype {
MainWindow::MainWindow(const QString &dataDirectory,QWidget *parent):QMainWindow(parent),
    settings_(dataDirectory.isEmpty()?QString():dataDirectory+"/preferences.ini"),
    history_(dataDirectory.isEmpty()?QString():dataDirectory+"/history.json") {
    initializeTypaTypeResources();preferences_=settings_.load();theme_.apply(preferences_.dark);
    setObjectName("mainWindow");setWindowTitle("TypaType — a little practice");setWindowIcon(QIcon(":/assets/paw.svg"));setMinimumSize(820,700);resize(1120,820);
    auto *shell=new QWidget;shell->setObjectName("shell");setCentralWidget(shell);auto *layout=new QVBoxLayout(shell);layout->setContentsMargins(42,25,42,25);layout->setSpacing(24);
    auto *header=new QHBoxLayout;header->setSpacing(8);auto *brand=new QLabel("TypaType");brand->setStyleSheet("font-size: 48px; font-weight: 700;");header->addWidget(brand);
    auto *paw=new QLabel;paw->setPixmap(QIcon(":/assets/paw.svg").pixmap(40,40));header->addWidget(paw);header->addStretch();
    auto *navGroup=new QButtonGroup(this);navGroup->setExclusive(true);const QStringList names{"Play","Progress","Settings"};
    for(int i=0;i<names.size();++i) {
        auto *b=new QPushButton(names[i]);b->setObjectName(names[i].toLower()+"NavButton");b->setProperty("role","nav");b->setCheckable(true);b->setCursor(Qt::PointingHandCursor);navGroup->addButton(b);nav_<<b;header->addWidget(b);
        connect(b,&QPushButton::clicked,this,[this,i]{navigate(i==0?0:i==1?2:3);});
    }
    layout->addLayout(header);notice_=new QLabel;notice_->setObjectName("saveNotice");notice_->setWordWrap(true);notice_->setProperty("role","muted");notice_->hide();layout->addWidget(notice_);
    sound_=new SoundManager(this);sound_->apply(preferences_);
    stack_=new QStackedWidget;stack_->setObjectName("pageStack");layout->addWidget(stack_,1);
    play_=new PlayPage(theme_,*sound_);results_=new ResultsPage;progress_=new ProgressPage(theme_);settingsPage_=new SettingsPage(preferences_,theme_);
    stack_->addWidget(play_);stack_->addWidget(results_);stack_->addWidget(progress_);stack_->addWidget(settingsPage_);
    connect(play_,&PlayPage::roundCompleted,this,&MainWindow::completeRound);
    connect(results_,&ResultsPage::playAgain,this,[this]{navigate(0);});connect(results_,&ResultsPage::viewProgress,this,[this]{navigate(2);});
    connect(settingsPage_,&SettingsPage::preferencesChanged,this,&MainWindow::applySettings);
    connect(settingsPage_,&SettingsPage::soundPreviewRequested,sound_,&SoundManager::preview);
    play_->applyPreferences(preferences_);nav_[0]->setChecked(true);
    showNotice(history_.load());progress_->refresh(history_.results());
    QTimer::singleShot(0,play_->board(),[this]{
        if(stack_->currentWidget()==play_&&!QApplication::activePopupWidget()&&!QApplication::activeModalWidget())
            play_->board()->setFocus();
    });
}
void MainWindow::showNotice(const QString &message) {notice_->setText(message);notice_->setVisible(!message.isEmpty());}
void MainWindow::navigate(int page) {
    if(page<0||page>=stack_->count())return;
    if(page==stack_->currentIndex()) {if(page==0)play_->board()->setFocus();return;}
    if(stack_->currentIndex()==0) play_->abandon();
    if(page==0) play_->prepare();
    if(page==2) {
        const auto warning=history_.load();if(!warning.isEmpty())showNotice(warning);
        progress_->refresh(history_.results());
    }
    stack_->setCurrentIndex(page);nav_[0]->setChecked(page==0||page==1);nav_[1]->setChecked(page==2);nav_[2]->setChecked(page==3);
    if(page==0)play_->board()->setFocus();
}
void MainWindow::completeRound(const RoundResult &r) {
    auto old=history_.results();
    QString error;bool saved=history_.append(r,error);
    if(!saved)showNotice(error);
    else {
        // append reloads any results from another app instance before saving.
        old=history_.results();
        const auto savedId=QUuid(r.id);
        old.erase(std::remove_if(old.begin(),old.end(),[&](const auto &o){return QUuid(o.id)==savedId;}),old.end());
        showNotice({});
    }
    const RoundResult *previous=old.empty()?nullptr:&old.back();
    bool speedBest=old.empty()||std::all_of(old.begin(),old.end(),[&](const auto &o){return r.score.wpm>o.score.wpm;});
    bool accuracyBest=old.empty()||std::all_of(old.begin(),old.end(),[&](const auto &o){return r.score.accuracy>o.score.accuracy;});
    // A best is announced only after this completed result has been safely saved.
    results_->showResult(r,previous,saved&&speedBest,saved&&accuracyBest,saved);
    stack_->setCurrentWidget(results_);nav_[0]->setChecked(true);
    results_->findChild<QPushButton*>("playAgainButton")->setFocus();
}
void MainWindow::applySettings() {
    preferences_=settingsPage_->preferences();theme_.apply(preferences_.dark);sound_->apply(preferences_);play_->applyPreferences(preferences_);
    if(!settings_.save(preferences_))showNotice("Your preferences could not be saved. They will still apply for this visit.");
    for(auto *widget:findChildren<QWidget*>())widget->update();
}
}
