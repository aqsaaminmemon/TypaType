#include "Stores.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>
#include <QUuid>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace typatype {
namespace {
QString dataDirectory() { return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation); }
QJsonObject encode(const RoundResult &r) {
    return {{"schemaVersion",1},{"roundId",r.id},{"completedAt",r.completedAt.toUTC().toString(Qt::ISODateWithMs)},
        {"difficulty",QString::fromStdString(difficultyKey(r.difficulty))},{"duration",r.duration},{"wordCount",r.wordCount},
        {"creditedCharacters",double(r.score.creditedCharacters)},{"totalCharacterAttempts",double(r.score.totalAttempts)},
        {"correctCharacterAttempts",double(r.score.correctAttempts)},{"wpm",r.score.wpm},{"accuracy",r.score.accuracy}};
}
bool integer(const QJsonObject &o, const char *key, double max, double &out) {
    const auto v=o.value(QLatin1String(key));
    if(!v.isDouble()) return false;
    out=v.toDouble(); return std::isfinite(out) && out>=0 && out<=max && std::floor(out)==out;
}
bool decode(const QJsonObject &o, RoundResult &r) {
    double version,duration,words,credited,attempts,correct;
    const double maxInteger=std::min(9007199254740991.0,double(std::numeric_limits<std::size_t>::max()));
    if(!integer(o,"schemaVersion",1,version)||version!=1 || !integer(o,"duration",60,duration) ||
       !integer(o,"wordCount",60,words) || !integer(o,"creditedCharacters",maxInteger,credited) ||
       !integer(o,"totalCharacterAttempts",maxInteger,attempts) || !integer(o,"correctCharacterAttempts",maxInteger,correct)) return false;
    if((duration!=15 && duration!=30 && duration!=60) || words!=duration || correct>attempts || credited>correct+std::min(words,correct)) return false;
    auto key=o.value("difficulty").toString();
    if(key=="simple") r.difficulty=Difficulty::Simple;
    else if(key=="punctuation") r.difficulty=Difficulty::Punctuation;
    else if(key=="punctuation_numbers") r.difficulty=Difficulty::Numbers;
    else return false;
    const QUuid id(o.value("roundId").toString());
    if(id.isNull()) return false;
    r.id=id.toString(QUuid::WithoutBraces);
    auto stamp=o.value("completedAt").toString();
    // Require an explicit timezone; local timestamps are ambiguous across computers.
    if(!stamp.endsWith('Z') && !stamp.contains(QRegularExpression("[+-]\\d{2}:\\d{2}$"))) return false;
    r.completedAt=QDateTime::fromString(stamp,Qt::ISODateWithMs);
    if(!r.completedAt.isValid()) return false;
    r.duration=int(duration); r.wordCount=int(words);
    if(!o.value("wpm").isDouble()||!o.value("accuracy").isDouble()) return false;
    double wpm=o.value("wpm").toDouble(),accuracy=o.value("accuracy").toDouble();
    if(!std::isfinite(wpm)||!std::isfinite(accuracy)||wpm<0||accuracy<0||accuracy>100) return false;
    const auto expectedWpm=(credited/5.0)/(duration/60.0);
    const auto expectedAccuracy=attempts ? 100.0*correct/attempts : 0.0;
    if(std::abs(wpm-expectedWpm)>1e-7 || std::abs(accuracy-expectedAccuracy)>1e-7) return false;
    r.score={std::size_t(credited),std::size_t(attempts),std::size_t(correct),wpm,accuracy};
    return true;
}
}
SettingsStore::SettingsStore(const QString &file): settings_(file.isEmpty() ? dataDirectory()+"/preferences.ini" : file,QSettings::IniFormat) {}
Preferences SettingsStore::load() const {
    Preferences p;
    p.dark=settings_.value("appearance/dark",false).toBool();
    bool ok=false; auto size=settings_.value("appearance/textSize",30).toInt(&ok);
    p.textSize=ok ? std::clamp(size,22,40) : 30;
    p.typingSound=settings_.value("sound/typing",true).toBool();
    p.celebrationSound=settings_.value("sound/celebration",true).toBool();
    auto volume=settings_.value("sound/volume",35).toInt(&ok);
    p.volume=ok ? std::clamp(volume,0,100) : 35;
    return p;
}
bool SettingsStore::save(const Preferences &p) {
    settings_.setValue("appearance/dark",p.dark); settings_.setValue("appearance/textSize",p.textSize);
    settings_.setValue("sound/typing",p.typingSound); settings_.setValue("sound/celebration",p.celebrationSound);
    settings_.setValue("sound/volume",p.volume); settings_.sync(); return settings_.status()==QSettings::NoError;
}
HistoryStore::HistoryStore(QString file): file_(QFileInfo(file.isEmpty()? dataDirectory()+"/history.json":std::move(file)).absoluteFilePath()) {}
QString HistoryStore::load() {
    rounds_.clear(); writable_=true; QFile file(file_);
    if(!file.exists()) return {};
    if(!file.open(QIODevice::ReadOnly)) {writable_=false;return "Your history could not be read. The existing file will be kept safe.";}
    QJsonParseError parse; auto document=QJsonDocument::fromJson(file.readAll(),&parse);
    if(file.error()!=QFileDevice::NoError || parse.error!=QJsonParseError::NoError || !document.isObject() ||
       document.object().value("schemaVersion")!=QJsonValue(1) || !document.object().value("rounds").isArray()) {
        writable_=false;return "Your history file could not be understood. It has been preserved; new rounds cannot be saved to it.";
    }
    int skipped=0; QSet<QString> ids;
    for(const auto &entry:document.object().value("rounds").toArray()) {
        RoundResult r;
        if(!entry.isObject() || !decode(entry.toObject(),r)||ids.contains(r.id)) {++skipped;continue;}
        ids.insert(r.id);rounds_.push_back(r);
    }
    std::stable_sort(rounds_.begin(),rounds_.end(),[](const auto &a,const auto &b){return a.completedAt<b.completedAt;});
    return skipped ? QString("Loaded your history; skipped %1 invalid or duplicate entries.").arg(skipped) : QString();
}
bool HistoryStore::append(const RoundResult &round, QString &error) {
    error.clear();
    RoundResult validated;
    if(!decode(encode(round),validated)) {error="This round could not be saved because its result data is invalid.";return false;}
    if(!QDir().mkpath(QFileInfo(file_).absolutePath())) {error="Your results folder could not be created. This round was not saved.";return false;}
    // Read and append under the same lock so another copy of TypaType cannot
    // overwrite rounds saved since startup. Never wait indefinitely in the UI.
    QLockFile lock(file_+".lock");lock.setStaleLockTime(0);
    if(!lock.tryLock(0)) {
        error=lock.error()==QLockFile::LockFailedError
            ? "Your history is being saved by another app window. This round could not be saved."
            : "Your history folder could not be written to. This round was not saved.";
        return false;
    }
    load();
    if(!writable_) {error="This round could not be saved because the existing history file is unreadable. Your original file is safe.";return false;}
    if(std::any_of(rounds_.begin(),rounds_.end(),[&](const auto &r){return r.id==validated.id;})) return true;
    auto updated=rounds_; updated.push_back(validated);
    std::stable_sort(updated.begin(),updated.end(),[](const auto &a,const auto &b){return a.completedAt<b.completedAt;});
    QJsonArray array; for(const auto &r:updated) array.append(encode(r));
    const auto bytes=QJsonDocument(QJsonObject{{"schemaVersion",1},{"rounds",array}}).toJson(QJsonDocument::Indented);
    QSaveFile file(file_); file.setDirectWriteFallback(false);
    if(!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size() || !file.commit()) {
        error="Your round could not be saved. Check the results folder permissions or available space.";return false;
    }
    rounds_=std::move(updated); return true;
}
std::vector<VocabularyEntry> loadVocabulary() {
    QFile file(":/assets/vocabulary.tsv");
    if(!file.open(QIODevice::ReadOnly)) throw std::runtime_error("Bundled vocabulary is missing");
    std::vector<VocabularyEntry> entries;
    for(auto line:QString::fromUtf8(file.readAll()).split('\n')) {
        if(line.startsWith('#')) continue;
        auto fields=line.trimmed().split('\t');
        if(fields.size()==2) entries.push_back({fields[0].toStdString(),fields[1].toStdString()});
    }
    return entries;
}
QString displayDifficulty(Difficulty d) { return QString::fromStdString(difficultyName(d)); }
QString comparisonText(const RoundResult &latest,const RoundResult *previous) {
    if(!previous) return "Your first recorded round";
    auto signedNumber=[](double n){return (n>=0?QString("+"):QString())+QString::number(n,'f',1);};
    return QString("%1 WPM · %2 percentage points accuracy\nCompared with your previous completed round")
        .arg(signedNumber(latest.score.wpm-previous->score.wpm),signedNumber(latest.score.accuracy-previous->score.accuracy));
}
}
