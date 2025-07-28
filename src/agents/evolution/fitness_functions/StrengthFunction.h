#pragma once

#include "AtomDB.h"
#include "FitnessFunction.h"

using namespace std;
using namespace atomdb;

namespace fitness_functions {
class StrengthFunction : public FitnessFunction {
   public:
    static string VARIABLE_NAME;

    StrengthFunction();
    ~StrengthFunction() {}

    float eval(shared_ptr<QueryAnswer> query_answer) override;

   private:
    shared_ptr<AtomDB> db;
};

}  // namespace fitness_functions