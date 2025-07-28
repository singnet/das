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
    if (query_answer->handles.size() != 1) {
        Utils::error("Invalid answer in StrengthFunction");
    } else {
        for (const auto& handle : query_answer->handles) {
            auto atom = db->get_atom(handle);
            LOG_INFO("Evaluating strength for handle: " << handle);
            LOG_INFO("MeTTa expression: " << atom->metta_representation(*db.get()));
            LOG_INFO("Atom document: " << atom->to_string());
            strength *= atom->custom_attributes.get<double>(VARIABLE_NAME);
        }
    }
    return strength;
}

