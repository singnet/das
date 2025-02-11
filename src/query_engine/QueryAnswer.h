#ifndef _QUERY_ENGINE_QUERYANSWER_H
#define _QUERY_ENGINE_QUERYANSWER_H

#include <string>

// If any of these constants are set to numbers greater than 999, we need
// to fix HandlesAnswer.tokenize() properly
#define MAX_VARIABLE_NAME_SIZE ((unsigned int) 100)
#define MAX_NUMBER_OF_VARIABLES_IN_QUERY ((unsigned int) 100)
#define MAX_NUMBER_OF_OPERATION_CLAUSES ((unsigned int) 100)

using namespace std;

namespace query_engine {

class QueryAnswer {
   public:
    QueryAnswer() = default;

    virtual ~QueryAnswer() = default;

    virtual const string& tokenize() = 0;

    virtual void untokenize(const string& tokens) = 0;

    virtual string to_string() = 0;
};

}  // namespace query_engine

#endif  // _QUERY_ENGINE_QUERYANSWER_H
