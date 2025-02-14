#pragma once

#include "QueryAnswer.h"
#include "QueryNode.h"

using namespace std;

namespace query_engine {

class QueryAnswerProcessor {
   public:
    virtual ~QueryAnswerProcessor() = default;
    virtual void process_answer(QueryAnswer* query_answer) = 0;
    virtual void query_answers_finished() = 0;
    virtual void graceful_shutdown() = 0;
};

}  // namespace query_engine
