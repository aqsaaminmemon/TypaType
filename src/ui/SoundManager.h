#pragma once
#include "storage/Stores.h"
#include <QElapsedTimer>
#include <QMediaDevices>
#include <QObject>
#include <QSoundEffect>
#include <array>
#include <memory>
namespace typatype {
class SoundManager : public QObject {
public:
    explicit SoundManager(QObject *parent=nullptr);
    void apply(const Preferences &preferences);
    void type() {play(0);}
    void advance() {play(1);}
    void erase() {play(2);}
    void encourage() {play(3);}
    void celebrate() {play(4);}
    void click() {play(5);}
    void preview() {if(enabled(3))celebrate();else if(enabled(0))advance();}
protected:
    bool eventFilter(QObject *object,QEvent *event) override;
private:
    struct Cue {
        // Short voices let rapid typing finish a click without dropping the next key.
        std::array<std::unique_ptr<QSoundEffect>,3> voices;
        std::size_t next=0;
        bool pending=false;
        QElapsedTimer requested;
    };
    bool enabled(std::size_t cue) const;
    void preload();
    void play(std::size_t cue);
    void dispatch(std::size_t cue);
    void updateAudioOutput();
    inline static constexpr std::array<const char*,6> names_{{"typing","word","backspace","encouragement","celebration","click"}};
    std::array<Cue,6> cues_;
    std::unique_ptr<QMediaDevices> devices_;
    Preferences preferences_;
    bool playbackAvailable_;
};
}
