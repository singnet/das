#define LOG_LEVEL DEBUG_LEVEL

#include "MultiplyStrengthFunction.h"

#include "AtomDBSingleton.h"
#include "Logger.h"
#include "Utils.h"

using namespace std;
using namespace atomdb;
using namespace fitness_functions;

string MultiplyStrengthFunction::VARIABLE_NAME = "strength";

MultiplyStrengthFunction::MultiplyStrengthFunction() { db = AtomDBSingleton::get_instance(); }

float MultiplyStrengthFunction::eval(shared_ptr<QueryAnswer> query_answer) {
    float strength = 1.0;

    LOG_DEBUG("Evaluating strength for " << query_answer->to_string());

    for (const auto& handle : query_answer->handles) {
        auto atom = db->get_atom(handle);
        LOG_DEBUG("Evaluating strength for handle: " << handle);
        LOG_DEBUG("MeTTa expression: " << atom->metta_representation(*db.get()));
        LOG_DEBUG("Atom document: " << atom->to_string());
        strength *= atom->custom_attributes.get<double>(VARIABLE_NAME);
    }
    LOG_DEBUG("Computed strength: " << strength);
    return strength;
}
