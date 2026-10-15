#include "ui/MainWindow.h"
#include <QApplication>
#include <QElapsedTimer>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
using namespace typatype;
class RealTimeTests:public QObject {
    Q_OBJECT
private slots:
    void realCountdown_data() {
        QTest::addColumn<int>("duration");QTest::addColumn<int>("difficulty");
        QTest::newRow("Simple-15s")<<15<<0;
        QTest::newRow("Punctuation-30s")<<30<<1;
        QTest::newRow("Numbers-60s")<<60<<2;
    }
    void realCountdown() {
        QFETCH(int,duration);QFETCH(int,difficulty);
        QTemporaryDir dir;MainWindow window(dir.path());window.show();QTest::qWait(40);
        auto *play=window.playPage();play->setOptions(Difficulty(difficulty),duration);
        auto &session=play->session();QStringList targets;
        for(const auto &word:session.words())targets<<QString::fromStdString(word.target);
        QSignalSpy completed(play,&PlayPage::roundCompleted);QElapsedTimer elapsed;elapsed.start();
        const auto expected=targets[0][0];const auto wrong=expected=='x'?QString("z"):QString("x");
        QTest::keyClicks(play->board(),wrong);QTest::keyClick(play->board(),Qt::Key_Backspace);
        QTest::keyClicks(play->board(),QString(expected));
        QCOMPARE(session.state(),SessionState::Running);
        window.findChild<QPushButton*>("durationSelector")->setFocus();QVERIFY(!play->board()->hasFocus());
        QTest::qWait(80);QVERIFY(session.remaining()<duration);QVERIFY(session.remaining()>duration-1);
        QVERIFY(completed.wait(duration*1000+1500));
        const auto measured=elapsed.elapsed();QVERIFY(measured>=duration*1000);QVERIFY(measured<=duration*1000+2000);
        QCOMPARE(session.state(),SessionState::Completed);QCOMPARE(completed.count(),1);QCOMPARE(window.history().results().size(),std::size_t(1));
        const auto &saved=window.history().results().back();QCOMPARE(saved.duration,duration);QCOMPARE(saved.score.creditedCharacters,std::size_t(1));
        QCOMPARE(saved.score.totalAttempts,std::size_t(2));QCOMPARE(saved.score.correctAttempts,std::size_t(1));QCOMPARE(saved.score.accuracy,50.0);
        QVERIFY(std::abs(saved.score.wpm-12.0/duration)<1e-9);
        QStringList after;for(const auto &word:session.words())after<<QString::fromStdString(word.target);QCOMPARE(after,targets);
        play->poll();QTest::keyClicks(play->board(),"ignored");QCOMPARE(completed.count(),1);QCOMPARE(session.score().totalAttempts,std::size_t(2));
        qInfo()<<"Actual countdown:"<<duration<<"seconds; measured"<<measured<<"ms; one saved result";
    }
};
int main(int argc,char **argv) {
    qputenv("TYPATYPE_DISABLE_AUDIO","1");QApplication app(argc,argv);
    app.setOrganizationName("TypaTypeTests");app.setApplicationName("TypaTypeTests");
    initializeTypaTypeResources();qRegisterMetaType<RoundResult>();RealTimeTests tests;return QTest::qExec(&tests,argc,argv);
}
#include "realtime_tests.moc"
