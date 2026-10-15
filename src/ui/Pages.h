#pragma once
#include "TypingWidget.h"
#include "MascotWidget.h"
#include "SoundManager.h"
#include <QCheckBox>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>
#include <functional>
namespace typatype {
class SelectionDialog : public QDialog {
    Q_OBJECT
public:
    SelectionDialog(const QString &title,const QStringList &options,int selected,QWidget *parent);
    int selection() const {return selection_;}
    void setCurrentChoice(int selected);
    bool shouldRestoreParentActivation() const {return restoreParent_;}
protected:
    void done(int result) override;
    void keyPressEvent(QKeyEvent*) override;
    bool eventFilter(QObject*,QEvent*) override;
private:
    int selection_=-1;
    bool outsideParent_=false,restoreParent_=false;
    std::vector<QPushButton*> choices_;
};
class PlayPage : public QWidget {
    Q_OBJECT
public:
    PlayPage(ThemeManager &theme,SoundManager &sound,QWidget *parent=nullptr);
    void prepare();
    void abandon();
    void applyPreferences(const Preferences &p);
    void poll();
    GameSession& session() {return session_;}
    TypingWidget* board() const {return board_;}
    void setOptions(Difficulty d,int duration);
signals:
    void roundCompleted(const RoundResult &round);
private:
    void chooseDifficulty();
    void chooseDuration();
    void restoreTypingFocus();
    void showSelector(SelectionDialog *&dialog,const QString &title,const QStringList &options,int selected,std::function<void(int)> apply);
    void refresh();
    WordGenerator generator_;
    GameSession session_;
    SoundManager &sound_;
    TypingWidget *board_;
    MascotWidget *mascot_;
    QLabel *cheer_;
    QPushButton *difficulty_,*duration_;
    SelectionDialog *difficultyDialog_=nullptr,*durationDialog_=nullptr;
    QTimer timer_;
    QTimer cheerTimer_;
    int cheerIndex_=0;
    bool delivered_=false;
    std::size_t milestone_=0;
};
class ResultsPage : public QWidget {
    Q_OBJECT
public:
    explicit ResultsPage(QWidget *parent=nullptr);
    void showResult(const RoundResult &round,const RoundResult *previous,bool bestWpm,bool bestAccuracy,bool saved);
signals:
    void playAgain();
    void viewProgress();
private:
    MascotWidget *mascot_;
    QLabel *wpm_,*accuracy_,*context_,*comparison_,*message_,*best_,*saved_;
};
class ProgressChart : public QWidget {
    Q_OBJECT
public:
    explicit ProgressChart(ThemeManager &theme,QWidget *parent=nullptr);
    void setResults(const std::vector<RoundResult>& rounds);
    void setAccuracy(bool accuracy);
    void showOlder();
    void showNewer();
    std::size_t visibleStart() const {return first_;}
    std::size_t visibleCount() const {return std::min(std::size_t(20),rounds_.size()-first_);}
    std::size_t totalCount() const {return rounds_.size();}
    QSize sizeHint() const override {return {800,275};}
signals:
    void pointDetailsChanged(const QString &details);
    void visibleRangeChanged();
protected:
    void paintEvent(QPaintEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void leaveEvent(QEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
private:
    int nearest(QPointF point) const;
    QString pointDetails(int index) const;
    void changeRange(std::size_t first);
    void showPoint(int index,QPoint global);
    ThemeManager &theme_;
    std::vector<RoundResult> rounds_;
    std::vector<QPointF> points_;
    std::vector<bool> speedBests_,accuracyBests_;
    std::size_t first_=0;
    bool accuracy_=false;
    int selected_=-1;
    int hovered_=-1;
};
class ProgressPage : public QWidget {
    Q_OBJECT
public:
    explicit ProgressPage(ThemeManager &theme,QWidget *parent=nullptr);
    void refresh(const std::vector<RoundResult>& rounds);
private:
    void updateRange();
    ProgressChart *chart_;
    QLabel *count_,*bestWpm_,*bestAccuracy_,*comparison_;
    QVBoxLayout *recent_;
    QLabel *range_,*pointDetails_;
    QPushButton *older_,*newer_;
};
class SettingsPage : public QWidget {
    Q_OBJECT
public:
    SettingsPage(const Preferences &preferences,ThemeManager &theme,QWidget *parent=nullptr);
    Preferences preferences() const;
signals:
    void preferencesChanged();
    void soundPreviewRequested();
private:
    void refreshPreview();
    QCheckBox *dark_,*typing_,*celebration_;
    QSlider *size_,*volume_;
    QLabel *preview_,*sizeValue_,*volumeValue_;
    QPushButton *testSound_;
    ThemeManager &theme_;
};
}
