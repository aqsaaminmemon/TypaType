#include "ui/MainWindow.h"
#include <QApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QtEndian>
#include <cmath>
#include <QUuid>
using namespace typatype;
namespace {
RoundResult result(int i=0) {
    RoundResult r{QUuid::createUuid().toString(QUuid::WithoutBraces),QDateTime::currentDateTimeUtc().addSecs(i),Difficulty(i%3),30,30,{}};
    r.score={std::size_t(50+i*3),std::size_t(100+i*3),std::size_t(80+i*3),0,0};
    r.score.wpm=(r.score.creditedCharacters/5.0)/.5;r.score.accuracy=100.0*r.score.correctAttempts/r.score.totalAttempts;return r;
}
void screenshot(QWidget &widget,const QString &name) {
    const auto folder=qEnvironmentVariable("TYPATYPE_SCREENSHOTS");if(folder.isEmpty())return;
    QDir().mkpath(folder);QTest::qWait(80);widget.grab().save(folder+"/"+name+".png");
}
SelectionDialog* activeSelection() {
    for(auto *widget:QApplication::topLevelWidgets()) if(auto *dialog=qobject_cast<SelectionDialog*>(widget);dialog&&dialog->isVisible())return dialog;
    return nullptr;
}
bool select(QWidget &window,const char *selector,int index,const QString &capture={}) {
    auto *anchor=window.findChild<QPushButton*>(selector);
    QTest::mouseClick(anchor,Qt::LeftButton);
    auto *dialog=activeSelection();if(!dialog)return false;
    const int anchorTop=anchor->mapToGlobal(QPoint()).y();
    const int belowGap=dialog->y()-(anchorTop+anchor->height());
    const int aboveGap=anchorTop-(dialog->y()+dialog->height());
    if(belowGap!=4&&aboveGap!=4){dialog->reject();return false;}
    if(!capture.isEmpty())screenshot(*dialog,capture);
    auto *choice=dialog->findChild<QPushButton*>(QString("choice_%1").arg(index));
    if(!choice){dialog->reject();return false;}
    choice->click();return true;

}
}
class DesktopTests:public QObject {
    Q_OBJECT
private slots:
    void historyRoundTrip() {
        QTemporaryDir dir;HistoryStore store(dir.path()+"/history.json");QVERIFY(store.load().isEmpty());QString error;
        auto r=result();QVERIFY(store.append(r,error));QVERIFY(store.append(r,error));QCOMPARE(store.results().size(),std::size_t(1));
        auto second=result(1);QVERIFY(store.append(second,error));HistoryStore loaded(store.fileName());QVERIFY(loaded.load().isEmpty());QCOMPARE(loaded.results().size(),std::size_t(2));
        QCOMPARE(loaded.results()[0].id,r.id);QCOMPARE(loaded.results()[1].difficulty,Difficulty::Punctuation);QCOMPARE(loaded.results()[0].score.wpm,r.score.wpm);
        QCOMPARE(loaded.results()[0].score.accuracy,r.score.accuracy);QVERIFY(comparisonText(r,nullptr).contains("first recorded"));QVERIFY(comparisonText(second,&r).contains("percentage points"));
    }
    void preservesUnreadableHistory() {
        QTemporaryDir dir;const auto path=dir.path()+"/history.json";QFile f(path);QVERIFY(f.open(QIODevice::WriteOnly));f.write("{broken history");f.close();
        HistoryStore store(path);QVERIFY(!store.load().isEmpty());QString error;QVERIFY(!store.append(result(),error));QVERIFY(!error.isEmpty());
        QVERIFY(f.open(QIODevice::ReadOnly));QCOMPARE(f.readAll(),QByteArray("{broken history"));f.close();
        const auto directory=dir.path()+"/directory";QDir().mkpath(directory);HistoryStore bad(directory);QVERIFY(!bad.load().isEmpty());QVERIFY(!bad.append(result(),error));
    }
    void rejectsInvalidEntries() {
        QTemporaryDir dir;QString path=dir.path()+"/history.json";HistoryStore store(path);store.load();QString error;QVERIFY(store.append(result(),error));
        QFile f(path);QVERIFY(f.open(QIODevice::ReadOnly));auto object=QJsonDocument::fromJson(f.readAll()).object();f.close();auto array=object["rounds"].toArray();auto valid=array[0].toObject();
        auto invalid=valid;invalid["roundId"]=QUuid::createUuid().toString();invalid["accuracy"]=101;array.append(invalid);array.append("invalid");array.append(valid);
        invalid=valid;invalid["roundId"]=QUuid::createUuid().toString();invalid["correctCharacterAttempts"]=-3;array.append(invalid);
        object["rounds"]=array;QVERIFY(f.open(QIODevice::WriteOnly|QIODevice::Truncate));f.write(QJsonDocument(object).toJson());f.close();
        HistoryStore loaded(path);QVERIFY(loaded.load().contains("4"));QCOMPARE(loaded.results().size(),std::size_t(1));
        auto invalidResult=result(2);invalidResult.score.wpm=999;QVERIFY(!loaded.append(invalidResult,error));
    }
    void historyChangesWhileAppIsOpen() {
        QTemporaryDir dir;const auto path=dir.path()+"/history.json";
        HistoryStore first(path),second(path);first.load();second.load();QString error;
        const auto a=result(),b=result(1);QVERIFY(first.append(a,error));QVERIFY(second.append(b,error));
        HistoryStore disk(path);QVERIFY(disk.load().isEmpty());QCOMPARE(disk.results().size(),std::size_t(2));
        // Equivalent UUID spellings must never create a second record.
        auto duplicate=a;duplicate.id=QUuid(a.id).toString().toUpper();QVERIFY(second.append(duplicate,error));QCOMPARE(second.results().size(),std::size_t(2));
        QLockFile lock(path+".lock");lock.setStaleLockTime(0);QVERIFY(lock.tryLock());
        QVERIFY(!first.append(result(2),error));QVERIFY(error.contains("another app window"));lock.unlock();
        QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly|QIODevice::Truncate));file.write("damaged after startup");file.close();
        QVERIFY(!first.append(result(3),error));QVERIFY(error.contains("unreadable"));
        QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(file.readAll(),QByteArray("damaged after startup"));file.close();
    }
    void milestoneOnlyOncePerRound() {
        QTemporaryDir dir;MainWindow window(dir.path());window.show();QTest::qWait(40);
        auto *play=window.playPage();auto *mascot=play->findChild<MascotWidget*>("playMascot");auto &session=play->session();
        for(int i=0;i<10;++i) {
            const auto target=QString::fromStdString(session.words()[i].target);
            QTest::keyClicks(play->board(),target);QTest::keyClick(play->board(),Qt::Key_Space);
        }
        QCOMPARE(session.lockedWords(),std::size_t(10));QCOMPARE(mascot->expression(),MascotExpression::Encouragement);
        // Timer delivery can be delayed by the desktop/event loop. Observe the
        // state change instead of assuming a fixed sleep is an exact deadline.
        QTRY_COMPARE_WITH_TIMEOUT(mascot->expression(),MascotExpression::Welcome,2200);
        play->poll();QCOMPARE(mascot->expression(),MascotExpression::Welcome);
        QTest::keyClick(play->board(),Qt::Key_Backspace);QCOMPARE(session.activeIndex(),std::size_t(10));
        play->prepare();QCOMPARE(session.lockedWords(),std::size_t(0));QCOMPARE(mascot->expression(),MascotExpression::Welcome);
    }
    void settingsRoundTrip() {
        QTemporaryDir dir;QString path=dir.path()+"/settings.ini";SettingsStore store(path);
        auto p=store.load();QVERIFY(!p.dark);QCOMPARE(p.textSize,30);QVERIFY(p.typingSound);QCOMPARE(p.volume,35);
        p={true,40,false,false,0};QVERIFY(store.save(p));SettingsStore other(path);auto loaded=other.load();
        QVERIFY(loaded.dark);QCOMPARE(loaded.textSize,40);QVERIFY(!loaded.typingSound);QVERIFY(!loaded.celebrationSound);QCOMPARE(loaded.volume,0);
    }
    void startupAndInput() {
        QTemporaryDir dir;MainWindow window(dir.path());window.show();QTest::qWait(80);auto *play=window.playPage();auto *board=play->board();auto &s=play->session();
        QCOMPARE(s.difficulty(),Difficulty::Simple);QCOMPARE(s.duration(),30);QCOMPARE(s.words().size(),std::size_t(30));QCOMPARE(s.state(),SessionState::Ready);QVERIFY(board->hasFocus());
        QCOMPARE(window.history().results().size(),std::size_t(0));screenshot(window,"play-light");
        QTest::keyClick(board,Qt::Key_Space);QTest::keyClick(board,Qt::Key_Return);QTest::keyClick(board,Qt::Key_Backspace);QTest::keyClick(board,Qt::Key_Shift);QTest::mouseClick(board,Qt::LeftButton);QCOMPARE(s.state(),SessionState::Ready);
        QTest::keyClick(board,Qt::Key_A,Qt::ControlModifier);QCOMPARE(s.state(),SessionState::Ready);
        const auto first=QString::fromStdString(s.words()[0].target);QTest::keyClicks(board,first);QCOMPARE(s.state(),SessionState::Running);QTest::keyClick(board,Qt::Key_Space);QVERIFY(s.words()[0].locked);
        QCOMPARE(s.score().totalAttempts,std::size_t(first.size()));QCOMPARE(s.score().creditedCharacters,std::size_t(first.size()+1));
        QTest::keyClick(board,Qt::Key_Space);QCOMPARE(s.activeIndex(),std::size_t(1));QVERIFY(s.words()[1].input.empty());
        QTest::keyClicks(board,"xxx");QTest::keyClick(board,Qt::Key_Space);QCOMPARE(s.activeIndex(),std::size_t(2));QTest::keyClick(board,Qt::Key_Backspace);QCOMPARE(s.activeIndex(),std::size_t(1));QCOMPARE(s.words()[1].input,U"xx");
        QTest::keyClick(board,Qt::Key_Backspace);QTest::keyClick(board,Qt::Key_Backspace);QTest::keyClick(board,Qt::Key_Backspace);QCOMPARE(s.activeIndex(),std::size_t(1));QVERIFY(s.words()[0].locked);
        QTest::keyClick(board,Qt::Key_Space);QCOMPARE(s.activeIndex(),std::size_t(1));QVERIFY(s.words()[1].input.empty());
        QTest::keyClicks(board,"x");screenshot(window,"play-feedback");
        auto old=s.words()[0].target;QTest::mouseClick(window.findChild<QPushButton*>("restartButton"),Qt::LeftButton);QCOMPARE(s.state(),SessionState::Ready);QVERIFY(s.words()[0].target!=old||s.words()[1].input.empty());QVERIFY(board->hasFocus());
    }
    void targetLettersStayVisible() {
        ThemeManager theme;theme.apply(false);GameSession session;TypingWidget board(session,theme);
        board.resize(700,300);board.show();board.setFocus();QTest::qWait(20);
        for(bool dark:{false,true}) {
            theme.apply(dark);
            auto render=[&](const char *input) {
                session.reset({"cat","dog"},Difficulty::Simple,30);board.resetView();
                QTest::keyClicks(&board,input);return board.grab().toImage();
            };
            const auto wrongX=render("x"),wrongZ=render("z");
            QCOMPARE(wrongX,wrongZ); // Same red target 'c', never the wrong 'x'/'z'.
            QCOMPARE(session.words()[0].target,std::string("cat"));QCOMPARE(session.words()[0].input,U"z");
            QCOMPARE(session.score().creditedCharacters,std::size_t(0));QCOMPARE(session.score().correctAttempts,std::size_t(0));
            int redPixels=0;
            for(int y=87;y<130;++y)for(int x=30;x<55;++x)
                if(wrongZ.pixelColor(x,y)==theme.colors().error)++redPixels;
            QVERIFY(redPixels>5);
            screenshot(board,dark?"target-feedback-dark":"target-feedback-light");
            QTest::keyClick(&board,Qt::Key_Backspace);QTest::keyClicks(&board,"c");
            QVERIFY(board.grab().toImage()!=wrongZ);QCOMPARE(session.words()[0].input,U"c");
            QCOMPARE(session.score().creditedCharacters,std::size_t(1));QCOMPARE(session.score().totalAttempts,std::size_t(2));
            screenshot(board,dark?"target-feedback-dark-corrected":"target-feedback-light-corrected");
        }
    }
    void keyboardCuesAndEndOfWords() {
        ThemeManager theme;theme.apply(false);GameSession session;session.reset({"cat","dog"},Difficulty::Simple,30);
        TypingWidget board(session,theme);board.show();
        QSignalSpy typing(&board,&TypingWidget::characterInserted),advance(&board,&TypingWidget::wordAdvanced),erase(&board,&TypingWidget::characterErased);
        QTest::keyClick(&board,Qt::Key_Space);QTest::keyClick(&board,Qt::Key_Backspace);
        QCOMPARE(typing.count(),0);QCOMPARE(advance.count(),0);QCOMPARE(erase.count(),0);
        QTest::keyClicks(&board,"cat");QTest::keyClick(&board,Qt::Key_Space);
        QTest::keyClick(&board,Qt::Key_Backspace);QCOMPARE(erase.count(),0); // Locked boundary.
        QTest::keyClicks(&board,"x");QTest::keyClick(&board,Qt::Key_Backspace);
        QTest::keyClicks(&board,"dog");QTest::keyClick(&board,Qt::Key_Return); // Optional alias.
        QCOMPARE(typing.count(),7);QCOMPARE(advance.count(),2);QCOMPARE(erase.count(),1);
        QCOMPARE(session.activeIndex(),std::size_t(2));QCOMPARE(session.score().totalAttempts,std::size_t(7));QCOMPARE(session.score().creditedCharacters,std::size_t(8));
        QTest::keyClick(&board,Qt::Key_Space);QTest::keyClicks(&board,"z");
        QCOMPARE(advance.count(),2);QCOMPARE(typing.count(),7);QCOMPARE(session.words().size(),std::size_t(2));
        session.tick(Clock::now()+std::chrono::seconds(30));
        QTest::keyClick(&board,Qt::Key_Space);QTest::keyClick(&board,Qt::Key_Backspace);QTest::keyClicks(&board,"q");
        QCOMPARE(advance.count(),2);QCOMPARE(erase.count(),1);QCOMPARE(typing.count(),7);
    }
    void backspaceRestoresNeutralLetters() {
        ThemeManager theme;theme.apply(false);GameSession session;TypingWidget board(session,theme);
        board.resize(700,300);board.show();board.setFocus();QTest::qWait(20);
        for(bool dark:{false,true}) {
            theme.apply(dark);
            for(const char *input:{"cax","ca"}) {
                session.reset({"cat","dog"},Difficulty::Simple,30);board.resetView();
                QTest::keyClicks(&board,input);QTest::keyClick(&board,Qt::Key_Space);
                QTest::keyClick(&board,Qt::Key_Backspace);
                QCOMPARE(session.activeIndex(),std::size_t(0));QVERIFY(!session.words()[0].submitted);
                const auto erased=board.grab().toImage();
                const auto remaining=QString::fromUcs4(session.words()[0].input.data(),qsizetype(session.words()[0].input.size()));
                const auto attempts=session.score().totalAttempts;
                screenshot(board,dark?"backspace-neutral-dark":"backspace-neutral-light");
                // Erasing correct or incorrect input must look just like typing
                // the remaining prefix: all erased positions are neutral grey.
                session.reset({"cat","dog"},Difficulty::Simple,30);board.resetView();
                QTest::keyClicks(&board,remaining);QCOMPARE(board.grab().toImage(),erased);
                QVERIFY(attempts>session.score().totalAttempts);
            }
            session.reset({"cat","dog"},Difficulty::Simple,30);board.resetView();
            QTest::keyClicks(&board,"cat");QTest::keyClick(&board,Qt::Key_Space);QTest::keyClick(&board,Qt::Key_Backspace);
            QCOMPARE(session.activeIndex(),std::size_t(1));QVERIFY(session.words()[0].locked);
        }
    }
    void bundledSounds() {
        for(const auto *name:{"typing","word","backspace","encouragement","celebration","click"}) {
            QFile file(QString(":/assets/%1.wav").arg(name));QVERIFY(file.open(QIODevice::ReadOnly));
            const auto wav=file.readAll();QVERIFY(wav.size()>44);QCOMPARE(wav.left(4),QByteArray("RIFF"));QCOMPARE(wav.mid(8,4),QByteArray("WAVE"));
            QCOMPARE(wav.mid(20,2),QByteArray::fromHex("0100")); // PCM, compatible with QSoundEffect.
            QCOMPARE(wav.mid(22,2),QByteArray::fromHex("0100")); // Mono.
            QCOMPARE(wav.mid(34,2),QByteArray::fromHex("1000")); // 16-bit samples.
            QVERIFY(wav.size()<25000); // Short, bundled cues.
            double peak=0,energy=0;
            for(qsizetype i=44;i+1<wav.size();i+=2) {
                const double sample=qFromLittleEndian<qint16>(wav.constData()+i)/32768.0;
                peak=std::max(peak,std::abs(sample));energy+=sample*sample;
            }
            const double rms=std::sqrt(energy/((wav.size()-44)/2));
            QVERIFY(peak>.4&&peak<.6);QVERIFY(rms>.1&&rms<.2);
        }
    }
    void upcomingLineIsVisible() {
        ThemeManager theme;theme.apply(false);GameSession session;TypingWidget board(session,theme);
        board.resize(420,300);board.show();board.setFocus();QTest::qWait(20);
        for(int size:{24,30,40}) {
            board.setTextSize(size);board.resize(420,board.minimumHeight());
            auto render=[&](char upcoming) {
                session.reset({std::string(14,'a'),std::string(14,'b'),std::string(14,'c'),std::string(14,upcoming)},Difficulty::Simple,30);
                for(int word=0;word<2;++word) {for(int i=0;i<14;++i)session.insert(word==0?'a':'b');session.enter();}
                board.resetView();return board.grab().toImage();
            };
            const auto first=render('d'),second=render('m');
            screenshot(board,QString("upcoming-line-%1").arg(size));
            // Changing only the following line must change the painted board.
            QVERIFY2(first!=second,qPrintable(QString("Upcoming text is hidden at text size %1").arg(size)));
            QTest::keyClick(&board,Qt::Key_Backspace);QCOMPARE(session.activeIndex(),std::size_t(2));
        }
    }
    void kittenCyclesMessagesAndFaces() {
        QTemporaryDir dir;MainWindow window(dir.path());window.show();QTest::qWait(40);
        auto *play=window.playPage();auto *mascot=play->findChild<MascotWidget*>("playMascot");
        auto *message=play->findChild<QLabel*>("kittenMessage");
        const auto welcome=message->text();const auto neutral=mascot->grab().toImage();
        QTRY_VERIFY_WITH_TIMEOUT(message->text()!=welcome,5500);
        QTRY_COMPARE_WITH_TIMEOUT(mascot->expression(),MascotExpression::Smile,1000);
        const auto smile=mascot->grab().toImage();QVERIFY(neutral!=smile);screenshot(window,"play-smiling-kitten");
        mascot->express(MascotExpression::Wink);QVERIFY(mascot->grab().toImage()!=smile);screenshot(*mascot,"kitten-wink");
        mascot->express(MascotExpression::Curious);QVERIFY(mascot->grab().toImage()!=smile);screenshot(*mascot,"kitten-curious");
        play->prepare();QCOMPARE(message->text(),welcome);QCOMPARE(mascot->expression(),MascotExpression::Welcome);
    }
    void soundPreviewFollowsSettings() {
        QTemporaryDir dir;MainWindow window(dir.path());window.show();window.navigate(3);
        auto *settings=window.findChild<SettingsPage*>("settingsPage");
        auto *button=window.findChild<QPushButton*>("testSoundButton");QVERIFY(button);
        QSignalSpy preview(settings,&SettingsPage::soundPreviewRequested);
        QVERIFY(button->isEnabled());button->click();QCOMPARE(preview.count(),1);
        auto *volume=window.findChild<QSlider*>("volumeSlider");volume->setValue(0);
        QVERIFY(!button->isEnabled());button->click();QCOMPARE(preview.count(),1);
        volume->setValue(35);window.findChild<QCheckBox*>("typingSoundCheck")->setChecked(false);
        window.findChild<QCheckBox*>("celebrationSoundCheck")->setChecked(false);QVERIFY(!button->isEnabled());
        window.findChild<QCheckBox*>("typingSoundCheck")->setChecked(true);QVERIFY(button->isEnabled());
        button->click();QCOMPARE(preview.count(),2);
        QVERIFY(window.history().results().empty());QCOMPARE(window.playPage()->session().state(),SessionState::Ready);
        screenshot(window,"settings-sound-preview");
    }
    void selectorsAndFocus() {
        QTemporaryDir dir;MainWindow window(dir.path());window.show();QTest::qWait(40);auto &s=window.playPage()->session();
        for(int difficulty=0;difficulty<3;++difficulty)for(int duration=0;duration<3;++duration) {
            QVERIFY(select(window,"difficultySelector",difficulty,difficulty==2&&duration==0?"difficulty-dialog":QString()));
            QVERIFY(select(window,"durationSelector",duration,difficulty==0&&duration==0?"duration-dialog":QString()));
            QCOMPARE(int(s.difficulty()),difficulty);QCOMPARE(s.duration(),QList<int>({15,30,60})[duration]);QCOMPARE(s.words().size(),std::size_t(s.duration()));QCOMPARE(s.state(),SessionState::Ready);
            QTRY_VERIFY_WITH_TIMEOUT(window.playPage()->board()->hasFocus(),500);
        }
        auto before=s.words();
        QTest::mouseClick(window.findChild<QPushButton*>("difficultySelector"),Qt::LeftButton);
        auto *dialog=activeSelection();QVERIFY(dialog);QTest::keyClick(dialog,Qt::Key_Escape);
        QCOMPARE(s.difficulty(),Difficulty::Numbers);QCOMPARE(s.words()[0].target,before[0].target);
        QTRY_VERIFY_WITH_TIMEOUT(window.playPage()->board()->hasFocus(),500);
        QTest::mouseClick(window.findChild<QPushButton*>("durationSelector"),Qt::LeftButton);
        dialog=activeSelection();QVERIFY(dialog);dialog->findChild<QPushButton*>("closeSelector")->click();QCOMPARE(s.duration(),60);
        QTRY_VERIFY_WITH_TIMEOUT(window.playPage()->board()->hasFocus(),500);
        QTest::mouseClick(window.findChild<QPushButton*>("durationSelector"),Qt::LeftButton);
        QVERIFY(activeSelection());QTest::mouseClick(&window,Qt::LeftButton,Qt::NoModifier,QPoint(5,5));QCOMPARE(s.duration(),60);
        QTRY_VERIFY_WITH_TIMEOUT(window.playPage()->board()->hasFocus(),500);
        QTest::mouseClick(window.findChild<QPushButton*>("durationSelector"),Qt::LeftButton);
        dialog=activeSelection();QVERIFY(dialog);QTest::keyClick(dialog,Qt::Key_Down);
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_Return);QCOMPARE(s.duration(),15);
        QTRY_VERIFY_WITH_TIMEOUT(window.playPage()->board()->hasFocus(),500);
        // An outside click into another app must not request parent activation.
        const auto unchanged=s.words()[0].target;
        QTest::mouseClick(window.findChild<QPushButton*>("durationSelector"),Qt::LeftButton);
        dialog=activeSelection();QVERIFY(dialog);
        QTest::mouseClick(&window,Qt::LeftButton,Qt::NoModifier,QPoint(-100,-100));
        QVERIFY(!dialog->shouldRestoreParentActivation());QVERIFY(!dialog->isVisible());
        QCOMPARE(s.duration(),15);QCOMPARE(s.words()[0].target,unchanged);
    }
    void navigationAndExpiry() {
        QTemporaryDir dir;MainWindow window(dir.path());window.show();QTest::qWait(40);auto *play=window.playPage();auto &s=play->session();
        QTest::keyClicks(play->board(),"mistake");window.navigate(2);QCOMPARE(window.history().results().size(),std::size_t(0));window.navigate(0);QCOMPARE(s.state(),SessionState::Ready);
        play->setOptions(Difficulty::Numbers,15);const auto start=Clock::now()-std::chrono::seconds(16);QVERIFY(s.insert(U'x',start));
        QSignalSpy completed(play,&PlayPage::roundCompleted);play->poll();QCOMPARE(completed.count(),1);play->poll();QCOMPARE(completed.count(),1);QCOMPARE(s.state(),SessionState::Completed);
        QCOMPARE(window.history().results().size(),std::size_t(1));QCOMPARE(window.findChild<QStackedWidget*>("pageStack")->currentIndex(),1);screenshot(window,"results-completed-test");
        QTest::keyClicks(play->board(),"more");QCOMPARE(window.history().results().size(),std::size_t(1));
        QTest::mouseClick(window.findChild<QPushButton*>("playAgainButton"),Qt::LeftButton);QCOMPARE(s.state(),SessionState::Ready);QCOMPARE(s.difficulty(),Difficulty::Numbers);QCOMPARE(s.duration(),15);QCOMPARE(s.words().size(),std::size_t(15));
        HistoryStore disk(dir.path()+"/history.json");QVERIFY(disk.load().isEmpty());QCOMPARE(disk.results().size(),std::size_t(1));
        MainWindow reopened(dir.path());QCOMPARE(reopened.playPage()->session().difficulty(),Difficulty::Simple);QCOMPARE(reopened.playPage()->session().duration(),30);QCOMPARE(reopened.history().results().size(),std::size_t(1));
    }
    void timerExpiresWhileSelectorIsOpen() {
        QTemporaryDir dir;MainWindow window(dir.path());window.show();QTest::qWait(40);
        auto *play=window.playPage();play->setOptions(Difficulty::Simple,15);
        QVERIFY(play->session().insert(U'x',Clock::now()-std::chrono::seconds(16)));
        QSignalSpy completed(play,&PlayPage::roundCompleted);bool dialogWasOpen=false;
        QTimer::singleShot(40,[&]{dialogWasOpen=activeSelection()!=nullptr;play->poll();});
        QTest::mouseClick(window.findChild<QPushButton*>("durationSelector"),Qt::LeftButton);
        QTRY_COMPARE_WITH_TIMEOUT(completed.count(),1,1000);QVERIFY(dialogWasOpen);QCOMPARE(window.findChild<QStackedWidget*>("pageStack")->currentIndex(),1);
        QCOMPARE(window.history().results().size(),std::size_t(1));QVERIFY(activeSelection()==nullptr);
    }
    void failedSaveKeepsResultsAndOriginalFile() {
        QTemporaryDir dir;const auto path=dir.path()+"/history.json";QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));file.write("original unreadable file");file.close();
        MainWindow window(dir.path());window.show();QTest::qWait(40);auto *play=window.playPage();
        const auto expected=static_cast<unsigned char>(play->session().words()[0].target[0]);
        QVERIFY(play->session().insert(expected,Clock::now()-std::chrono::seconds(31)));play->poll();
        QCOMPARE(window.findChild<QStackedWidget*>("pageStack")->currentIndex(),1);QCOMPARE(window.history().results().size(),std::size_t(0));
        QVERIFY(window.findChild<QLabel*>("saveNotice")->text().contains("unreadable"));
        for(auto *text:window.findChild<ResultsPage*>("resultsPage")->findChildren<QLabel*>())QVERIFY(!text->text().contains("A new personal best"));
        QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(file.readAll(),QByteArray("original unreadable file"));
    }
    void progressSettingsAndResize() {
        QTemporaryDir dir;MainWindow window(dir.path());window.show();QTest::qWait(40);window.navigate(2);screenshot(window,"progress-empty");
        window.navigate(3);window.findChild<QCheckBox*>("darkThemeCheck")->setChecked(true);window.findChild<QSlider*>("textSizeSlider")->setValue(40);
        window.findChild<QCheckBox*>("typingSoundCheck")->setChecked(false);window.findChild<QCheckBox*>("celebrationSoundCheck")->setChecked(false);window.findChild<QSlider*>("volumeSlider")->setValue(0);
        screenshot(window,"settings-dark");window.navigate(0);window.resize(820,700);QTest::qWait(40);screenshot(window,"play-dark-minimum");
        auto *board=window.playPage()->board();QVERIFY(board->width()>700);QVERIFY(board->height()>=240);
        auto *difficulty=window.findChild<QPushButton*>("difficultySelector"),*duration=window.findChild<QPushButton*>("durationSelector");
        auto d=difficulty->mapTo(window.centralWidget(),QPoint());auto t=duration->mapTo(window.centralWidget(),QPoint());QVERIFY(d.x()+difficulty->width()<t.x());
        SettingsStore saved(dir.path()+"/preferences.ini");auto p=saved.load();QVERIFY(p.dark);QCOMPARE(p.textSize,40);QCOMPARE(p.volume,0);QVERIFY(!p.typingSound);QVERIFY(!p.celebrationSound);
        MainWindow reopened(dir.path());QCOMPARE(reopened.playPage()->session().duration(),30);
    }
    void chartRangeAndPointDetails() {
        QTemporaryDir dir;HistoryStore history(dir.path()+"/history.json");history.load();QString error;
        std::vector<RoundResult> rounds;
        for(int i=0;i<45;++i) {auto r=result(i);rounds.push_back(r);QVERIFY(history.append(r,error));}
        MainWindow window(dir.path());window.show();window.navigate(2);QTest::qWait(60);
        auto *chart=window.findChild<ProgressChart*>("progressChart");auto *details=window.findChild<QLabel*>("chartPointDetails");
        auto *older=window.findChild<QPushButton*>("chartOlderButton"),*newer=window.findChild<QPushButton*>("chartNewerButton");
        QCOMPARE(chart->visibleStart(),std::size_t(25));QCOMPARE(chart->visibleCount(),std::size_t(20));QVERIFY(older->isEnabled());QVERIFY(!newer->isEnabled());
        chart->setFocus();QTest::keyClick(chart,Qt::Key_Home);QVERIFY(details->text().contains(QString::number(rounds[25].score.wpm,'f',1)+" WPM"));
        QTest::keyClick(chart,Qt::Key_End);QVERIFY(details->text().contains(QString::number(rounds[44].score.wpm,'f',1)+" WPM"));
        QTest::mouseClick(older,Qt::LeftButton);QCOMPARE(chart->visibleStart(),std::size_t(5));QVERIFY(newer->isEnabled());
        QTest::mouseClick(older,Qt::LeftButton);QCOMPARE(chart->visibleStart(),std::size_t(0));QVERIFY(!older->isEnabled());
        QTest::keyClick(chart,Qt::Key_Home);QVERIFY(details->text().contains("20.0 WPM"));
        QTest::mouseClick(newer,Qt::LeftButton);QCOMPARE(chart->visibleStart(),std::size_t(20));
        QTest::mouseClick(newer,Qt::LeftButton);QCOMPARE(chart->visibleStart(),std::size_t(25));
        QTest::keyClick(chart,Qt::Key_End);const auto remembered=details->text();
        QEvent leave(QEvent::Leave);QApplication::sendEvent(chart,&leave);QCOMPARE(details->text(),remembered);
        QTest::mouseClick(window.findChild<QPushButton*>("chartAccuracyButton"),Qt::LeftButton);QTest::keyClick(chart,Qt::Key_Home);QVERIFY(details->text().contains("accuracy"));
        screenshot(window,"progress-details-fixture");
    }
    void singleRoundChart() {
        QTemporaryDir dir;HistoryStore history(dir.path()+"/history.json");history.load();QString error;QVERIFY(history.append(result(),error));
        MainWindow window(dir.path());window.show();window.navigate(2);QTest::qWait(40);
        auto *chart=window.findChild<ProgressChart*>("progressChart");QCOMPARE(chart->visibleCount(),std::size_t(1));
        chart->setFocus();QTest::keyClick(chart,Qt::Key_Right);QVERIFY(window.findChild<QLabel*>("chartPointDetails")->text().contains("20.0 WPM"));
        QVERIFY(!window.findChild<QPushButton*>("chartOlderButton")->isEnabled());QVERIFY(!window.findChild<QPushButton*>("chartNewerButton")->isEnabled());
        screenshot(window,"progress-single-fixture");
    }
    void progressFixtures() {
        QTemporaryDir dir;HistoryStore history(dir.path()+"/history.json");history.load();QString error;
        for(int i=0;i<25;++i)QVERIFY(history.append(result(i),error));
        MainWindow window(dir.path());window.show();window.navigate(2);QTest::qWait(60);screenshot(window,"progress-fixture-wpm");
        QTest::mouseClick(window.findChild<QPushButton*>("chartAccuracyButton"),Qt::LeftButton);screenshot(window,"progress-fixture-accuracy");
        QCOMPARE(window.history().results().size(),std::size_t(25));
        window.resize(820,700);QTest::qWait(40);screenshot(window,"progress-fixture-minimum");
    }
};
int main(int argc,char **argv) {
    qputenv("TYPATYPE_DISABLE_AUDIO","1");
    QApplication app(argc,argv);app.setOrganizationName("TypaTypeTests");app.setApplicationName("TypaTypeTests");
    qRegisterMetaType<RoundResult>();initializeTypaTypeResources();DesktopTests tests;return QTest::qExec(&tests,argc,argv);
}
#include "desktop_tests.moc"
