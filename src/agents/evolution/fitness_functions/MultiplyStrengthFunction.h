#pragma once

#include "FitnessFunction.h"

using namespace std;

namespace fitness_functions {
class MultiplyStrengthFunction : public FitnessFunction {
   public:
    static string VARIABLE_NAME;

    MultiplyStrengthFunction() {}
    ~MultiplyStrengthFunction() {}

    float eval(shared_ptr<QueryAnswer> query_answer, shared_ptr<Keychain> keychain = nullptr) override;
};

}  // namespace fitness_functions
