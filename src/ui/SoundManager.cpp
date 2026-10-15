#include "SoundManager.h"
#include <QAudioDevice>
#include <QAbstractButton>
#include <QApplication>
#include <QDebug>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimer>
namespace typatype {
SoundManager::SoundManager(QObject *parent):QObject(parent),playbackAvailable_(!qEnvironmentVariableIsSet("TYPATYPE_DISABLE_AUDIO")) {
    qApp->installEventFilter(this);
    // Prepare resources after window construction, before the first keypress.
    if(playbackAvailable_)QTimer::singleShot(0,this,&SoundManager::preload);
}
bool SoundManager::enabled(std::size_t cue) const {
    return preferences_.volume>0 && (cue<3||cue==5?preferences_.typingSound:preferences_.celebrationSound);
}
bool SoundManager::eventFilter(QObject *object,QEvent *event) {
    auto *button=qobject_cast<QAbstractButton*>(object);
    auto *owner=qobject_cast<QWidget*>(parent());
    if(button&&button->isEnabled()&&(!owner||owner==button||owner->isAncestorOf(button))) {
        if(event->type()==QEvent::MouseButtonPress) {
            auto *mouse=static_cast<QMouseEvent*>(event);
            if(mouse->button()==Qt::LeftButton&&button->rect().contains(mouse->position().toPoint()))click();
        } else if(event->type()==QEvent::KeyPress) {
            auto *key=static_cast<QKeyEvent*>(event);
            if(!key->isAutoRepeat()&&(key->key()==Qt::Key_Space||key->key()==Qt::Key_Return||key->key()==Qt::Key_Enter))click();
        }
    }
    return QObject::eventFilter(object,event);
}
void SoundManager::apply(const Preferences &preferences) {
    preferences_=preferences;
    for(std::size_t i=0;i<cues_.size();++i) {
        if(!enabled(i))cues_[i].pending=false;
        for(auto &effect:cues_[i].voices)if(effect) {
            effect->setVolume(preferences.volume/100.0);
            if(!enabled(i))effect->stop();
        }
    }
    if(playbackAvailable_)QTimer::singleShot(0,this,&SoundManager::preload);
}
void SoundManager::preload() {
    if(!playbackAvailable_)return;
    if(!enabled(0)&&!enabled(3))return;
    if(!devices_) {
        devices_=std::make_unique<QMediaDevices>();
        connect(devices_.get(),&QMediaDevices::audioOutputsChanged,this,&SoundManager::updateAudioOutput);
    }
    const auto output=QMediaDevices::defaultAudioOutput();
    for(std::size_t i=0;i<cues_.size();++i)if(enabled(i)) {
        const std::size_t count=(i==0||i==2||i==5)?3:1;
        for(std::size_t v=0;v<count;++v)if(!cues_[i].voices[v]) {
            auto effect=std::make_unique<QSoundEffect>(output,this);
            auto *raw=effect.get();cues_[i].voices[v]=std::move(effect);
            raw->setObjectName(QString("%1Sound_%2").arg(names_[i]).arg(v));
            raw->setVolume(preferences_.volume/100.0);raw->setLoopCount(1);
            connect(raw,&QSoundEffect::statusChanged,this,[this,raw,i]{
                if(raw->status()==QSoundEffect::Ready)dispatch(i);
                else if(raw->status()==QSoundEffect::Error)qWarning()<<"Could not load TypaType sound:"<<raw->source();
            });
            raw->setSource(QUrl("qrc:/assets/"+QString::fromLatin1(names_[i])+".wav"));
        }
    }
}
void SoundManager::play(std::size_t cue) {
    if(!playbackAvailable_||!enabled(cue))return;
    cues_[cue].pending=true;cues_[cue].requested.start();
    dispatch(cue);
}
void SoundManager::dispatch(std::size_t cue) {
    auto &c=cues_[cue];
    if(!c.pending)return;
    if(!enabled(cue)||c.requested.elapsed()>180) {c.pending=false;return;}
    const std::size_t count=(cue==0||cue==2||cue==5)?3:1;
    for(std::size_t offset=0;offset<count;++offset) {
        const auto v=(c.next+offset)%count;
        auto &effect=c.voices[v];
        if(!effect||effect->status()!=QSoundEffect::Ready)continue;
        c.pending=false;c.next=(v+1)%count;
        // Restart a busy voice instead of skipping a new input.
        if(effect->isPlaying())effect->stop();
        effect->play();return;
    }
}
void SoundManager::updateAudioOutput() {
    const auto output=QMediaDevices::defaultAudioOutput();
    for(auto &cue:cues_) {
        cue.pending=false;
        for(auto &effect:cue.voices)if(effect) {
            effect->stop();effect->setAudioDevice(output);
        }
    }
}
}
