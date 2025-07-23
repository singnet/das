#include "UnitTestFunction.h"

using namespace fitness_functions;

float UnitTestFunction::eval(shared_ptr<QueryAnswer> query_answer) { return query_answer->importance; }
