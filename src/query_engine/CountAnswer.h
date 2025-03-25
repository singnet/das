#ifndef _QUERY_ENGINE_COUNTANSWER_H
#define _QUERY_ENGINE_COUNTANSWER_H

#include <string>

#include "QueryAnswer.h"

#define UNDEFINED_COUNT ((int) -1)

using namespace std;

namespace query_engine {

class CountAnswer : public QueryAnswer {
   public:
    CountAnswer(int count) : count(count){};

    CountAnswer() : count(UNDEFINED_COUNT){};

    ~CountAnswer(){};

    const string& tokenize() override {
        this->token_representation = std::to_string(this->count);
        return this->token_representation;
    }

    void untokenize(const string& tokens) override { this->count = std::stoi(tokens); }

    string to_string() override { return "CountAnswer<" + std::to_string(this->count) + ">"; };

    int get_count() { return count; }

   private:
    int count;
    string token_representation;
};

}  // namespace query_engine

#endif  // _QUERY_ENGINE_COUNTANSWER_H
