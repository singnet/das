#pragma once

#include "FitnessFunction.h"

using namespace std;
using namespace atomdb;

namespace fitness_functions {

class CountLetterFunction : public FitnessFunction {
   public:
    static string VARIABLE_NAME;
    static char LETTER_TO_COUNT;

    CountLetterFunction() {}
    ~CountLetterFunction() {}

    float eval(shared_ptr<QueryAnswer> query_answer, shared_ptr<Keychain> keychain = nullptr) override;
};

}  // namespace fitness_functions
