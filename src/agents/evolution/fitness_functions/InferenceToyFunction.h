#pragma once

#include "AtomDB.h"
#include "FitnessFunction.h"

using namespace std;
using namespace atomdb;

namespace fitness_functions {
class InferenceToyFunction : public FitnessFunction {
   public:
    static string STRENGTH_TAG;

    InferenceToyFunction();
    ~InferenceToyFunction() {}

    float eval(shared_ptr<QueryAnswer> query_answer, shared_ptr<Keychain> keychain = nullptr) override;

   private:
    shared_ptr<AtomDB> db;
};

}  // namespace fitness_functions
