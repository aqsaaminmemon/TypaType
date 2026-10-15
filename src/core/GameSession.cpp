#include "GameSession.h"
#include <algorithm>
#include <stdexcept>
namespace typatype {
bool WordState::exact() const {
    return input.size() == target.size() && std::equal(input.begin(), input.end(), target.begin());
}
std::string difficultyName(Difficulty d) {
    switch(d) {
    case Difficulty::Simple: return "Simple";
    case Difficulty::Punctuation: return "Punctuation";
    case Difficulty::Numbers: return "Punctuation + Numbers";
    }
    return "Simple";
}
std::string difficultyKey(Difficulty d) {
    switch(d) {
    case Difficulty::Simple: return "simple";
    case Difficulty::Punctuation: return "punctuation";
    case Difficulty::Numbers: return "punctuation_numbers";
    }
    return "simple";
}
void GameSession::reset(std::vector<std::string> targets, Difficulty d, int seconds) {
    if (seconds != 15 && seconds != 30 && seconds != 60) throw std::invalid_argument("Unsupported duration");
    if (targets.empty()) throw std::invalid_argument("Empty word set");
    words_.clear();
    for (auto &t : targets) {
        if(t.empty()) throw std::invalid_argument("Empty target word");
        words_.push_back({std::move(t), {}, false, false});
    }
    difficulty_ = d; duration_ = seconds; active_ = attempts_ = correct_ = 0;
    state_ = SessionState::Ready; started_ = {};
}
bool GameSession::tick(Clock::time_point now) {
    if (state_ == SessionState::Running && now - started_ >= std::chrono::seconds(duration_)) {
        state_ = SessionState::Completed;
        return true;
    }
    return false;
}
bool GameSession::insert(char32_t ch, Clock::time_point now) {
    tick(now);
    if (state_ == SessionState::Completed || active_ >= words_.size() || ch < U' ' || ch == 127 || words_[active_].locked) return false;
    if (state_ == SessionState::Ready) { state_ = SessionState::Running; started_ = now; }
    auto &word = words_[active_];
    const auto position = word.input.size();
    ++attempts_;
    if(position < word.target.size() && ch == static_cast<unsigned char>(word.target[position])) ++correct_;
    word.input.push_back(ch);
    return true;
}
bool GameSession::enter(Clock::time_point now) {
    tick(now);
    if(state_ != SessionState::Running || active_ >= words_.size() || words_[active_].input.empty()) return false;
    auto &word = words_[active_];
    word.submitted = true; word.locked = word.exact(); ++active_;
    return true;
}
bool GameSession::backspace(Clock::time_point now) {
    tick(now);
    if(state_ != SessionState::Running) return false;
    if(active_ < words_.size() && !words_[active_].input.empty()) {
        if(words_[active_].locked) return false;
        // Editing reopens submission feedback: erased positions are untouched
        // again until the word is submitted, while attempt accuracy is retained.
        words_[active_].input.pop_back();words_[active_].submitted=false;return true;
    }
    if(active_ == 0 || words_[active_ - 1].locked) return false;
    --active_;
    if(!words_[active_].input.empty()) words_[active_].input.pop_back();
    words_[active_].submitted=false;
    return true;
}
double GameSession::remaining(Clock::time_point now) const {
    if(state_ == SessionState::Ready) return duration_;
    if(state_ == SessionState::Completed) return 0;
    return std::max(0.0, duration_ - std::chrono::duration<double>(now - started_).count());
}
std::size_t GameSession::lockedWords() const {
    return std::count_if(words_.begin(), words_.end(), [](const WordState &w){return w.locked;});
}
Score GameSession::score() const {
    Score result;
    result.totalAttempts = attempts_; result.correctAttempts = correct_;
    for(const auto &word : words_) {
        for(std::size_t i = 0; i < std::min(word.input.size(), word.target.size()); ++i)
            if(word.input[i] == static_cast<unsigned char>(word.target[i])) ++result.creditedCharacters;
        if(word.submitted && word.exact()) ++result.creditedCharacters;
    }
    result.wpm = (result.creditedCharacters / 5.0) / (duration_ / 60.0);
    result.accuracy = attempts_ ? (100.0 * correct_ / attempts_) : 0.0;
    return result;
}
}
