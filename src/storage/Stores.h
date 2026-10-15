#pragma once
#include "core/GameSession.h"
#include "core/WordGenerator.h"
#include <QDateTime>
#include <QSettings>
#include <QString>
#include <vector>
namespace typatype {
struct RoundResult {
    QString id;
    QDateTime completedAt;
    Difficulty difficulty = Difficulty::Simple;
    int duration = 30;
    int wordCount = 30;
    Score score;
};
struct Preferences {
    bool dark = false;
    int textSize = 30;
    bool typingSound = true;
    bool celebrationSound = true;
    int volume = 35;
};
class SettingsStore {
public:
    explicit SettingsStore(const QString &file = {});
    Preferences load() const;
    bool save(const Preferences &preferences);
    QString fileName() const { return settings_.fileName(); }
private:
    mutable QSettings settings_;
};
class HistoryStore {
public:
    explicit HistoryStore(QString file = {});
    QString load();
    bool append(const RoundResult &round, QString &error);
    const std::vector<RoundResult>& results() const { return rounds_; }
    QString fileName() const { return file_; }
private:
    QString file_;
    std::vector<RoundResult> rounds_;
    bool writable_ = true;
};
std::vector<VocabularyEntry> loadVocabulary();
QString displayDifficulty(Difficulty d);
QString comparisonText(const RoundResult &latest, const RoundResult *previous);
}
