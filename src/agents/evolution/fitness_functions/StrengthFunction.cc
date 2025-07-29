#include "StrengthFunction.h"
#include "AtomDBSingleton.h"
#include "Utils.h"
#include "Logger.h"

using namespace std;
using namespace atomdb;
using namespace fitness_functions;


string StrengthFunction::VARIABLE_NAME = "strength";

StrengthFunction::StrengthFunction() {
    db = AtomDBSingleton::get_instance();
}

float StrengthFunction::eval(shared_ptr<QueryAnswer> query_answer) {
    float strength = 1.0;

    LOG_INFO("Evaluating strength for " << query_answer->to_string());
    LOG_INFO("Evaluating strength for " << query_answer->handles.size() << " handles.");

        for (const auto& handle : query_answer->handles) {
            auto atom = db->get_atom(handle);
            DEBUG_LEVEL("Evaluating strength for handle: " << handle);
            DEBUG_LEVEL("MeTTa expression: " << atom->metta_representation(*db.get()));
            DEBUG_LEVEL("Atom document: " << atom->to_string());
            strength *= atom->custom_attributes.get<double>(VARIABLE_NAME);
        }
    LOG_INFO("Computed strength: " << strength);
    return strength;
}

