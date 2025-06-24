#pragma once

#include "FitnessFunction.h"
#include "AtomDB.h"

using namespace std;
using namespace atomdb;

namespace fitness_functions {

class CountLetterFunction : public FitnessFunction {
   public:
    CountLetterFunction();
    ~CountLetterFunction() {}

    float eval(shared_ptr<QueryAnswer> query_answer) override;

   private:

    shared_ptr<AtomDB> db;

};

}  // namespace fitness_functions
