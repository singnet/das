#include "UnitTestFunction.h"

using namespace fitness_functions;

float UnitTestFunction::eval(shared_ptr<QueryAnswer> query_answer, shared_ptr<Keychain> keychain) { return query_answer->importance; }
