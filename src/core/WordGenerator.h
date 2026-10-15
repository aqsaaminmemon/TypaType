#pragma once
#include "GameSession.h"
#include <random>
namespace typatype {
struct VocabularyEntry { std::string category; std::string word; };
class WordGenerator {
public:
    explicit WordGenerator(std::vector<VocabularyEntry> vocabulary, unsigned seed = std::random_device{}());
    std::vector<std::string> generate(Difficulty difficulty, int duration);
private:
    std::mt19937 random_;
    std::vector<std::string> all_, helpers_, adjectives_, previous_;
    std::string choose(const std::vector<std::string>& pool, const std::string& previous);
};
}
