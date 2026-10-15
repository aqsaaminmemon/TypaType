#include "WordGenerator.h"
#include <algorithm>
#include <stdexcept>
namespace typatype {
namespace {
bool contains(const std::vector<std::string> &pool,const std::string &word) {
    return std::find(pool.begin(),pool.end(),word)!=pool.end();
}
void deduplicate(std::vector<std::string> &pool) {
    std::sort(pool.begin(),pool.end());pool.erase(std::unique(pool.begin(),pool.end()),pool.end());
}
std::string baseWord(const std::string &token) {
    std::string base;for(char ch:token)if(ch>='a'&&ch<='z')base+=ch;return base;
}
}
WordGenerator::WordGenerator(std::vector<VocabularyEntry> entries,unsigned seed):random_(seed) {
    for(const auto &entry:entries) {
        if(entry.word.empty() || !std::all_of(entry.word.begin(),entry.word.end(),[](char c){return c>='a'&&c<='z';}))continue;
        all_.push_back(entry.word);
        if(entry.category=="helper")helpers_.push_back(entry.word);
        if(entry.category=="adjective")adjectives_.push_back(entry.word);
    }
    deduplicate(all_);deduplicate(helpers_);deduplicate(adjectives_);
    if(all_.size()<2 || helpers_.size()<2 || adjectives_.size()<2)
        throw std::invalid_argument("Vocabulary needs distinct words, helping verbs, and adjectives");
}
std::string WordGenerator::choose(const std::vector<std::string> &pool,const std::string &previous) {
    std::uniform_int_distribution<std::size_t> pick(0,pool.size()-1);
    for(int retry=0;retry<100;++retry) {auto word=pool[pick(random_)];if(word!=previous)return word;}
    for(const auto &word:pool)if(word!=previous)return word;
    throw std::runtime_error("Vocabulary has no distinct words");
}
std::vector<std::string> WordGenerator::generate(Difficulty difficulty,int duration) {
    if(duration!=15 && duration!=30 && duration!=60)throw std::invalid_argument("Unsupported duration");
    std::vector<std::string> words;std::string last;
    for(int i=0;i<duration;++i) {
        const auto &pool=i%6==0?helpers_:i%6==1?adjectives_:all_;
        last=choose(pool,last);words.push_back(last);
    }
    std::shuffle(words.begin(),words.end(),random_);
    auto categoryPool=[&](const std::string &word)->const std::vector<std::string>& {
        return contains(helpers_,word)?helpers_:contains(adjectives_,word)?adjectives_:all_;
    };
    // Repair duplicates within the original category to preserve its frequency.
    for(std::size_t i=1;i<words.size();++i)
        if(words[i]==words[i-1])words[i]=choose(categoryPool(words[i]),words[i-1]);
    if(difficulty==Difficulty::Numbers) {
        const int minimum=(duration+9)/10;
        int helpers=int(std::count_if(words.begin(),words.end(),[&](const auto &w){return contains(helpers_,w);}));
        int adjectives=int(std::count_if(words.begin(),words.end(),[&](const auto &w){return contains(adjectives_,w);}));
        std::uniform_int_distribution<int> number(1,999),offset(0,4);
        auto numeric=[](const std::string &word){return word[0]>='0'&&word[0]<='9';};
        for(int planned=offset(random_);planned<duration;planned+=7) {
            for(int shift=0;shift<duration;++shift) {
                const int i=(planned+shift)%duration;
                bool helper=contains(helpers_,words[i]),adjective=contains(adjectives_,words[i]);
                if(numeric(words[i]) || (i>0&&numeric(words[i-1])) || (i+1<duration&&numeric(words[i+1])))continue;
                if((helper&&helpers<=minimum)||(adjective&&adjectives<=minimum))continue;
                words[i]=std::to_string(number(random_));helpers-=helper;adjectives-=adjective;break;
            }
        }
    }
    if(difficulty!=Difficulty::Simple) {
        const std::vector<std::pair<std::string,std::string>> punctuation={{"",","},{"","."},{"","!"},{"","?"},{"'","'"},{"\"","\""},{"(",")"}};
        std::uniform_int_distribution<int> choice(0,int(punctuation.size())-1),offset(0,3);
        for(int i=offset(random_);i<duration;i+=4) {
            const auto &mark=punctuation[choice(random_)];words[i]=mark.first+words[i]+mark.second;
        }
    }
    if(words==previous_) {
        bool replaced=false;
        for(std::size_t i=0;i<words.size()&&!replaced;++i) {
            auto base=baseWord(words[i]);if(base.empty())continue;
            for(const auto &candidate:categoryPool(base)) {
                auto replacement=candidate+(difficulty==Difficulty::Simple?"":"!");
                if(replacement==words[i] || (i&&candidate==baseWord(words[i-1])) || (i+1<words.size()&&candidate==baseWord(words[i+1])))continue;
                words[i]=replacement;replaced=true;break;
            }
        }
        if(!replaced)throw std::runtime_error("Vocabulary cannot produce a fresh sequence");
    }
    previous_=words;return words;
}
}
