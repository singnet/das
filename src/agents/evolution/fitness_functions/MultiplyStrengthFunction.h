#pragma once

#include "AtomDB.h"
#include "FitnessFunction.h"

using namespace std;
using namespace atomdb;

namespace fitness_functions {
class MultiplyStrengthFunction : public FitnessFunction {
   public:
    static string VARIABLE_NAME;

    MultiplyStrengthFunction();
    ~MultiplyStrengthFunction() {}

    float eval(shared_ptr<QueryAnswer> query_answer) override;

   private:
    shared_ptr<AtomDB> db;
};

}  // namespace fitness_functions