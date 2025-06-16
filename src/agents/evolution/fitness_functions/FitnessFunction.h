#pragma once

#include <memory>

#include "QueryAnswer.h"

using namespace std;
using namespace query_engine;

namespace fitness_functions {

/**
 * Abstract superclass for fitness functions used to evolve queries.
 */
class FitnessFunction {
   public:
    FitnessFunction(){};
    virtual ~FitnessFunction(){};

    virtual float eval(shared_ptr<QueryAnswer> query_answer) = 0;
};

}  // namespace fitness_functions
