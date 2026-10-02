#pragma once

#include "FitnessFunction.h"

using namespace std;

namespace fitness_functions {
class InferenceToyFunction : public FitnessFunction {
   public:
    static string STRENGTH_TAG;

    InferenceToyFunction() {}
    ~InferenceToyFunction() {}

    float eval(shared_ptr<QueryAnswer> query_answer, shared_ptr<Keychain> keychain = nullptr) override;
};

}  // namespace fitness_functions
