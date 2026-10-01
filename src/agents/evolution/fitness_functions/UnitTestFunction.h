#pragma once

#include "FitnessFunction.h"

using namespace std;

namespace fitness_functions {

class UnitTestFunction : public FitnessFunction {
   public:
    UnitTestFunction() {}
    ~UnitTestFunction() {}

    float eval(shared_ptr<QueryAnswer> query_answer, shared_ptr<Keychain> keychain = nullptr) override;

   private:
};

}  // namespace fitness_functions
