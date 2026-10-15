#pragma once
#include <chrono>
#include <cstddef>
#include <string>
#include <vector>
namespace typatype {
enum class Difficulty { Simple, Punctuation, Numbers };
enum class SessionState { Ready, Running, Completed };
using Clock = std::chrono::steady_clock;
struct WordState {
    std::string target;
    std::u32string input;
    bool submitted = false;
    bool locked = false;
    bool exact() const;
};
struct Score {
    std::size_t creditedCharacters = 0;
    std::size_t totalAttempts = 0;
    std::size_t correctAttempts = 0;
    double wpm = 0;
    double accuracy = 0;
};
class GameSession {
public:
    void reset(std::vector<std::string> targets, Difficulty difficulty, int duration);
    bool insert(char32_t ch, Clock::time_point now = Clock::now());
    bool enter(Clock::time_point now = Clock::now());
    bool backspace(Clock::time_point now = Clock::now());
    // Returns true only on the Running -> Completed transition.
    bool tick(Clock::time_point now = Clock::now());
    double remaining(Clock::time_point now = Clock::now()) const;
    Score score() const;
    SessionState state() const { return state_; }
    const std::vector<WordState>& words() const { return words_; }
    std::size_t activeIndex() const { return active_; }
    std::size_t lockedWords() const;
    int duration() const { return duration_; }
    Difficulty difficulty() const { return difficulty_; }
private:
    std::vector<WordState> words_;
    SessionState state_ = SessionState::Ready;
    Difficulty difficulty_ = Difficulty::Simple;
    int duration_ = 30;
    std::size_t active_ = 0, attempts_ = 0, correct_ = 0;
    Clock::time_point started_{};
};
std::string difficultyName(Difficulty difficulty);
std::string difficultyKey(Difficulty difficulty);
}
