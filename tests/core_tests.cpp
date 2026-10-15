#include "core/GameSession.h"
#include "core/WordGenerator.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace typatype;
static int checks = 0;
#define CHECK(x) do { ++checks; if(!(x)) throw std::runtime_error(std::string(__FILE__)+":"+std::to_string(__LINE__)+" " #x); } while(0)
static auto t0 = Clock::time_point{} + std::chrono::seconds(100);
void type(GameSession& s, const std::string& text) {for(char c:text) CHECK(s.insert(c,t0));}
void sessionTests() {
    GameSession s; s.reset({"cat","dog","moon"},Difficulty::Simple,30);
    CHECK(s.state()==SessionState::Ready); CHECK(s.duration()==30);
    CHECK(!s.enter(t0)); CHECK(!s.backspace(t0)); CHECK(s.remaining(t0)==30);
    CHECK(!s.insert(U'\n',t0)); CHECK(s.state()==SessionState::Ready);
    type(s,"cat"); CHECK(s.state()==SessionState::Running); CHECK(s.enter(t0)); CHECK(s.words()[0].locked);
    type(s,"dpg"); CHECK(s.enter(t0)); CHECK(!s.words()[1].locked);
    CHECK(s.backspace(t0)); CHECK(s.activeIndex()==1); CHECK(s.words()[1].input==U"dp");
    CHECK(s.backspace(t0)); type(s,"og"); CHECK(s.enter(t0)); CHECK(s.words()[1].locked);
    CHECK(!s.backspace(t0)); CHECK(s.activeIndex()==2);
    auto score=s.score(); CHECK(score.totalAttempts==8); CHECK(score.correctAttempts==7);
    CHECK(score.creditedCharacters==8); CHECK(std::abs(score.wpm-3.2)<1e-9); CHECK(score.accuracy==87.5);
    type(s,"mo"); CHECK(s.score().creditedCharacters==10);
    CHECK(!s.tick(t0+std::chrono::milliseconds(29999))); CHECK(s.tick(t0+std::chrono::seconds(30)));
    CHECK(!s.tick(t0+std::chrono::seconds(31))); CHECK(!s.insert(U'o',t0+std::chrono::seconds(31)));
    CHECK(!s.backspace(t0+std::chrono::seconds(31))); CHECK(!s.enter(t0+std::chrono::seconds(31)));
    CHECK(s.score().creditedCharacters==10); CHECK(s.remaining()==0);
    s.reset({"cat","dog","moon"},Difficulty::Simple,15);
    type(s,"cxt"); CHECK(s.enter(t0)); type(s,"dpg"); CHECK(s.enter(t0));
    CHECK(s.backspace(t0)); CHECK(s.backspace(t0)); CHECK(s.backspace(t0)); CHECK(s.backspace(t0));
    CHECK(s.activeIndex()==0); CHECK(s.words()[0].input==U"cx"); CHECK(s.words()[1].input.empty());
    CHECK(s.backspace(t0)); type(s,"at"); CHECK(s.enter(t0)); CHECK(s.words()[0].locked);
    type(s,"dog"); CHECK(s.enter(t0)); type(s,"moon"); CHECK(s.enter(t0));
    CHECK(s.activeIndex()==3); CHECK(!s.insert(U'x',t0)); CHECK(!s.enter(t0)); CHECK(!s.backspace(t0));
    CHECK(s.state()==SessionState::Running); CHECK(s.words().size()==3);
    CHECK(s.tick(t0+std::chrono::seconds(15)));
    s.reset({"cat","dog","moon"},Difficulty::Simple,30);
    type(s,"cx"); CHECK(s.enter(t0)); type(s,"dp"); CHECK(s.enter(t0));
    CHECK(s.backspace(t0)); CHECK(s.words()[1].input==U"d"); CHECK(s.backspace(t0)); CHECK(s.backspace(t0));
    CHECK(s.activeIndex()==0); CHECK(s.insert(U'a',t0)); CHECK(s.enter(t0));
    CHECK(s.words()[1].input.empty()); // revisited empty input retained
    type(s,"dog"); CHECK(s.enter(t0)); CHECK(!s.backspace(t0)); // lock forms boundary
    s.reset({"cat","dog"},Difficulty::Simple,30);
    type(s,"ca"); CHECK(s.enter(t0)); type(s,"dpg"); CHECK(s.enter(t0));
    CHECK(s.backspace(t0)); CHECK(s.backspace(t0)); CHECK(s.backspace(t0)); CHECK(s.backspace(t0));
    CHECK(s.activeIndex()==0); CHECK(s.words()[0].input==U"c");
    type(s,"at"); CHECK(s.enter(t0)); CHECK(s.words()[1].input.empty());
    s.reset({"cat","dog"},Difficulty::Simple,30);
    type(s,"cat dog"); CHECK(s.activeIndex()==0); CHECK(s.score().totalAttempts==7); CHECK(s.score().correctAttempts==3);
    for(int i=0;i<4;++i) CHECK(s.backspace(t0)); CHECK(s.score().creditedCharacters==3);
    CHECK(s.enter(t0)); CHECK(s.score().creditedCharacters==4);
    s.reset({"cat"},Difficulty::Simple,15); CHECK(s.score().accuracy==0);
    CHECK(s.insert(U'x',t0)); CHECK(s.backspace(t0)); type(s,"cat");
    CHECK(s.score().creditedCharacters==3); CHECK(s.score().accuracy==75);
    CHECK(s.enter(t0)); CHECK(s.score().creditedCharacters==4);
    s.reset({"cat"},Difficulty::Simple,15); type(s,"c");
    CHECK(!s.insert(U'a',t0+std::chrono::seconds(15))); CHECK(s.state()==SessionState::Completed);
    s.reset({"cat"},Difficulty::Simple,60); type(s,"x"); CHECK(s.enter(t0));
    CHECK(s.backspace(t0)); CHECK(s.activeIndex()==0); CHECK(s.words()[0].input.empty());
    CHECK(s.state()==SessionState::Running); CHECK(!s.enter(t0)); CHECK(s.words().size()==1);
    // Later nonempty text survives revisiting an earlier mistake.
    s.reset({"cat","dog","moon"},Difficulty::Simple,30);
    type(s,"cx"); CHECK(s.enter(t0)); type(s,"dog"); CHECK(s.backspace(t0)); CHECK(s.backspace(t0)); CHECK(s.backspace(t0));
    CHECK(s.backspace(t0)); type(s,"at"); CHECK(s.enter(t0)); CHECK(s.words()[1].input.empty());
    // A previously submitted word is not erased by re-submitting its predecessor.
    s.reset({"cat","dog","moon"},Difficulty::Simple,30);
    type(s,"cx"); CHECK(s.enter(t0)); type(s,"dpg"); CHECK(s.enter(t0));
    CHECK(s.backspace(t0)); CHECK(s.words()[1].input==U"dp"); CHECK(s.enter(t0)); CHECK(s.words()[0].input==U"cx");
    s.reset({"(cat)","27"},Difficulty::Numbers,15); type(s,"(cat)"); CHECK(s.enter(t0)); type(s,"27");
    CHECK(s.score().creditedCharacters==8); CHECK(s.score().accuracy==100);
}
void generationTests() {
    std::ifstream file(VOCABULARY_PATH); CHECK(file.good());
    std::vector<VocabularyEntry> entries; std::string line;
    while(std::getline(file,line)) {auto p=line.find('\t'); if(p!=std::string::npos&&line[0]!='#') entries.push_back({line.substr(0,p),line.substr(p+1)});}
    CHECK(entries.size()>200); WordGenerator generator(entries,123456);
    for(auto d:{Difficulty::Simple,Difficulty::Punctuation,Difficulty::Numbers}) for(int duration:{15,30,60}) {
        std::vector<std::string> previous;
        for(int round=0;round<100;++round) {
            auto words=generator.generate(d,duration); CHECK(words.size()==std::size_t(duration)); CHECK(words!=previous);
            bool punct=false,number=false; int helpers=0,adjectives=0;
            for(std::size_t i=0;i<words.size();++i) {
                if(i) CHECK(words[i]!=words[i-1]);
                std::string base;for(char c:words[i])if(c>='a'&&c<='z')base+=c;
                for(const auto &e:entries)if(e.word==base){if(e.category=="helper")++helpers;if(e.category=="adjective")++adjectives;}
                for(char c:words[i]) {if(c>='0'&&c<='9') number=true; else if(!(c>='a'&&c<='z')) punct=true;}
            }
            CHECK(helpers>=(duration+9)/10);CHECK(adjectives>=(duration+9)/10);CHECK(punct==(d!=Difficulty::Simple)); CHECK(number==(d==Difficulty::Numbers)); previous=words;
        }
    }
}
void smallVocabularyTests() {
    const std::vector<VocabularyEntry> minimal={{"helper","is"},{"helper","are"},{"adjective","soft"},{"adjective","kind"}};
    for(unsigned seed=0;seed<80;++seed) {
        WordGenerator generator(minimal,seed);
        for(int duration:{15,30,60})for(auto difficulty:{Difficulty::Simple,Difficulty::Punctuation,Difficulty::Numbers}) {
            auto words=generator.generate(difficulty,duration);
            int helpers=0,adjectives=0,numbers=0;bool punctuation=false;
            for(std::size_t i=0;i<words.size();++i) {
                std::string base;for(char ch:words[i]) {
                    if(ch>='a'&&ch<='z')base+=ch;
                    else if(ch>='0'&&ch<='9')++numbers;
                    else punctuation=true;
                }
                if(base=="is"||base=="are")++helpers;
                if(base=="soft"||base=="kind")++adjectives;
                if(i)CHECK(words[i]!=words[i-1]);
            }
            CHECK(helpers>=(duration+9)/10);CHECK(adjectives>=(duration+9)/10);
            CHECK((numbers>0)==(difficulty==Difficulty::Numbers));CHECK(punctuation==(difficulty!=Difficulty::Simple));
        }
    }
    bool rejected=false;
    try {WordGenerator invalid({{"helper","is"},{"helper","is"},{"adjective","soft"},{"adjective","soft"}},1);}
    catch(const std::invalid_argument&){rejected=true;}
    CHECK(rejected);
}
int main() {try {sessionTests();generationTests();smallVocabularyTests();std::cout<<checks<<" checks passed\n";return 0;} catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
