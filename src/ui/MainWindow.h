#pragma once
#include "Pages.h"
#include <QMainWindow>
#include <QStackedWidget>
#include <QStatusBar>
void initializeTypaTypeResources();
namespace typatype {
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const QString &dataDirectory={},QWidget *parent=nullptr);
    void navigate(int page);
    PlayPage* playPage() const {return play_;}
    const HistoryStore& history() const {return history_;}
    void showNotice(const QString &message);
private:
    void completeRound(const RoundResult &result);
    void applySettings();
    ThemeManager theme_;
    SettingsStore settings_;
    HistoryStore history_;
    Preferences preferences_;
    SoundManager *sound_;
    QStackedWidget *stack_;
    PlayPage *play_;
    ResultsPage *results_;
    ProgressPage *progress_;
    SettingsPage *settingsPage_;
    QLabel *notice_;
    QList<QPushButton*> nav_;
};
}
