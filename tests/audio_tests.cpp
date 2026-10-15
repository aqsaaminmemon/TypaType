// Run manually on a desktop with an audio output: ./build/audio_tests
// Kept outside default CTest because CI/offscreen machines may lack audio.
#include "ui/MainWindow.h"
#include <QAudioDevice>
#include <QApplication>
#include <QSignalSpy>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSet>
#include <QScopeGuard>
#include <QStandardPaths>
#include <QTest>
#include <QtEndian>
#include <cmath>
using namespace typatype;
class AudioTests:public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        initializeTypaTypeResources();
        QVERIFY2(!QMediaDevices::defaultAudioOutput().isNull(),"A desktop audio output is required");
    }
    void preloadedSilentAndResponsive() {
        SoundManager sound;Preferences p;p.volume=20;sound.apply(p);
        QTRY_COMPARE_WITH_TIMEOUT(sound.findChildren<QSoundEffect*>().size(),12,3000);
        for(auto *effect:sound.findChildren<QSoundEffect*>()) {
            QTRY_COMPARE_WITH_TIMEOUT(effect->status(),QSoundEffect::Ready,3000);
            QVERIFY(!effect->isPlaying()); // Preloading must not produce a sound.
            QCOMPARE(effect->volume(),.2f);
        }
        for(const auto *name:{"typing","word","backspace","encouragement","celebration","click"}) {
            auto *effect=sound.findChild<QSoundEffect*>(QString("%1Sound_0").arg(name));QVERIFY(effect);
            if(QString(name)=="typing")sound.type();
            else if(QString(name)=="word")sound.advance();
            else if(QString(name)=="backspace")sound.erase();
            else if(QString(name)=="encouragement")sound.encourage();
            else if(QString(name)=="celebration")sound.celebrate();
            else sound.click();
            QVERIFY(effect->isPlaying());effect->stop();
        }
        // Requests while a cue is already playing must not be discarded.
        auto *word=sound.findChild<QSoundEffect*>("wordSound_0");
        QSignalSpy transitions(word,&QSoundEffect::playingChanged);
        sound.advance();sound.advance();QCOMPARE(transitions.count(),3); // Play, stop, play.
        for(auto *effect:sound.findChildren<QSoundEffect*>())effect->stop();
        for(int i=0;i<3;++i)sound.type();
        for(int i=0;i<3;++i)QVERIFY(sound.findChild<QSoundEffect*>(QString("typingSound_%1").arg(i))->isPlaying());
        sound.type(); // All voices busy: still play this key.
        for(auto *effect:sound.findChildren<QSoundEffect*>())effect->stop();
    }
    void muteAndVolume() {
        SoundManager sound;Preferences p;p.volume=20;sound.apply(p);
        QTRY_COMPARE_WITH_TIMEOUT(sound.findChildren<QSoundEffect*>().size(),12,3000);
        for(auto *effect:sound.findChildren<QSoundEffect*>())QTRY_COMPARE_WITH_TIMEOUT(effect->status(),QSoundEffect::Ready,3000);
        sound.type();sound.celebrate();p.typingSound=false;sound.apply(p);
        sound.type();sound.advance();sound.erase();
        for(auto *effect:sound.findChildren<QSoundEffect*>())if(!effect->objectName().startsWith("celebration"))QVERIFY(!effect->isPlaying());
        QVERIFY(sound.findChild<QSoundEffect*>("celebrationSound_0")->isPlaying());
        p.volume=0;sound.apply(p);sound.encourage();sound.celebrate();
        for(auto *effect:sound.findChildren<QSoundEffect*>()) {QVERIFY(!effect->isPlaying());QCOMPARE(effect->volume(),0.f);}
        p.typingSound=true;p.volume=30;sound.apply(p);sound.erase();
        QVERIFY(sound.findChild<QSoundEffect*>("backspaceSound_0")->isPlaying());
        for(auto *effect:sound.findChildren<QSoundEffect*>())effect->stop();
        p.celebrationSound=false;sound.apply(p);sound.preview();
        QVERIFY(sound.findChild<QSoundEffect*>("wordSound_0")->isPlaying());
        QVERIFY(!sound.findChild<QSoundEffect*>("celebrationSound_0")->isPlaying());
    }
    void buttonsPlayClickSound() {
        QWidget owner;QPushButton button("Click",&owner);owner.resize(180,100);button.resize(100,40);owner.show();
        SoundManager sound(&owner);Preferences p;p.volume=20;sound.apply(p);
        QTRY_COMPARE_WITH_TIMEOUT(sound.findChildren<QSoundEffect*>().size(),12,3000);
        auto *first=sound.findChild<QSoundEffect*>("clickSound_0");
        auto *second=sound.findChild<QSoundEffect*>("clickSound_1");
        auto *third=sound.findChild<QSoundEffect*>("clickSound_2");
        for(auto *effect:{first,second,third}) {QVERIFY(effect);QTRY_COMPARE_WITH_TIMEOUT(effect->status(),QSoundEffect::Ready,3000);}
        QTest::mousePress(&button,Qt::LeftButton);QVERIFY(first->isPlaying());QTest::mouseRelease(&button,Qt::LeftButton);
        QTest::keyPress(&button,Qt::Key_Space);QVERIFY(second->isPlaying());QTest::keyRelease(&button,Qt::Key_Space);
        p.typingSound=false;sound.apply(p);QTest::mouseClick(&button,Qt::LeftButton);
        for(auto *effect:{first,second,third})QVERIFY(!effect->isPlaying());
    }
    void cuesFinishOnDesktopOutput() {
        SoundManager sound;Preferences p;p.volume=20;sound.apply(p);
        QTRY_COMPARE_WITH_TIMEOUT(sound.findChildren<QSoundEffect*>().size(),12,3000);
        for(auto *effect:sound.findChildren<QSoundEffect*>())QTRY_COMPARE_WITH_TIMEOUT(effect->status(),QSoundEffect::Ready,3000);
        for(int cue=0;cue<6;++cue) {
            const QStringList names{"typing","word","backspace","encouragement","celebration","click"};
            auto *effect=sound.findChild<QSoundEffect*>(names[cue]+"Sound_0");
            switch(cue) {
                case 0:sound.type();break;case 1:sound.advance();break;
                case 2:sound.erase();break;case 3:sound.encourage();break;case 4:sound.celebrate();break;case 5:sound.click();break;
            }
            QVERIFY(effect->isPlaying());
            QTRY_VERIFY_WITH_TIMEOUT(!effect->isPlaying(),2000);
            QCOMPARE(effect->status(),QSoundEffect::Ready);
        }
    }
    void mutedWhileLoadingDoesNotReplay() {
        SoundManager sound;Preferences p;p.volume=20;sound.apply(p);
        sound.type();p.typingSound=false;p.celebrationSound=false;sound.apply(p);
        QTest::qWait(200);QVERIFY(sound.findChildren<QSoundEffect*>().isEmpty());
        p.typingSound=true;p.celebrationSound=true;sound.apply(p);
        QTRY_COMPARE_WITH_TIMEOUT(sound.findChildren<QSoundEffect*>().size(),12,3000);
        for(auto *effect:sound.findChildren<QSoundEffect*>()) {
            QTRY_COMPARE_WITH_TIMEOUT(effect->status(),QSoundEffect::Ready,3000);QVERIFY(!effect->isPlaying());
        }
    }
    void samplesReachDesktopOutput() {
        if(QStandardPaths::findExecutable("pactl").isEmpty()||QStandardPaths::findExecutable("parec").isEmpty())
            QSKIP("This output-sample check requires PulseAudio/PipeWire tools");
        SoundManager sound;Preferences p;p.volume=35;sound.apply(p);
        QTRY_COMPARE_WITH_TIMEOUT(sound.findChildren<QSoundEffect*>().size(),12,3000);
        auto *effect=sound.findChild<QSoundEffect*>("wordSound_0");
        QTRY_COMPARE_WITH_TIMEOUT(effect->status(),QSoundEffect::Ready,3000);
        effect->setLoopCount(QSoundEffect::Infinite);sound.advance();QTest::qWait(100);
        QProcess recorder;
        auto cleanup=qScopeGuard([&]{effect->stop();if(recorder.state()!=QProcess::NotRunning) {recorder.terminate();recorder.waitForFinished(1000);}});
        QProcess list;list.start("pactl",{"--format=json","list","sink-inputs"});QVERIFY(list.waitForFinished(2000));
        qInfo()<<"Qt audio output"<<QMediaDevices::defaultAudioOutput().description()<<QMediaDevices::defaultAudioOutput().id();
        int stream=-1;
        for(const auto &item:QJsonDocument::fromJson(list.readAllStandardOutput()).array()) {
            const auto entry=item.toObject();const auto properties=entry["properties"].toObject();
            if(properties["application.process.id"].toVariant().toString()==QString::number(QCoreApplication::applicationPid()) && !entry["corked"].toBool()) {
                stream=entry["index"].toInt();break;
            }
        }
        if(stream<0&&!QStandardPaths::findExecutable("pw-dump").isEmpty()) {
            // Native PipeWire streams omit the PulseAudio process property.
            // Resolve their client using its server-verified process ID.
            QProcess dump;dump.start("pw-dump",{});QVERIFY(dump.waitForFinished(2000));
            const auto objects=QJsonDocument::fromJson(dump.readAllStandardOutput()).array();QSet<qint64> clients;
            for(const auto &object:objects) {
                const auto entry=object.toObject(),props=entry["info"].toObject()["props"].toObject();
                if(props["pipewire.sec.pid"].toVariant().toLongLong()==QCoreApplication::applicationPid())clients.insert(entry["id"].toVariant().toLongLong());
            }
            for(const auto &object:objects) {
                const auto entry=object.toObject(),props=entry["info"].toObject()["props"].toObject();
                if(clients.contains(props["client.id"].toVariant().toLongLong())&&props["media.class"].toString()=="Stream/Output/Audio") {
                    stream=int(props["object.serial"].toVariant().toLongLong());break;
                }
            }
        }
        QVERIFY2(stream>=0,"No playing game stream reached the desktop audio server");
        // Monitor only this test's stream, never microphone or other apps.
        recorder.start("parec",{QString("--monitor-stream=%1").arg(stream),"--format=s16le","--rate=48000","--channels=1","--latency-msec=10"});
        QVERIFY(recorder.waitForStarted(1000));QTest::qWait(600);
        const auto pcm=recorder.readAllStandardOutput();QVERIFY2(pcm.size()>2000,qPrintable(recorder.readAllStandardError()));
        double peak=0,energy=0;
        for(qsizetype i=0;i+1<pcm.size();i+=2) {
            const double sample=qFromLittleEndian<qint16>(pcm.constData()+i)/32768.0;
            peak=std::max(peak,std::abs(sample));energy+=sample*sample;
        }
        const double rms=std::sqrt(energy/(pcm.size()/2));
        qInfo()<<"Game-only desktop output at 35% volume: peak"<<peak<<"RMS"<<rms;
        QVERIFY2(peak>.1 && rms>.03,"The game reports playing but output is silent or too quiet at the default game volume");
    }
};
int main(int argc,char **argv) {
    qunsetenv("TYPATYPE_DISABLE_AUDIO");QApplication app(argc,argv);
    app.setOrganizationName("TypaType");app.setApplicationName("TypaType");
    AudioTests tests;return QTest::qExec(&tests,argc,argv);
}
#include "audio_tests.moc"
