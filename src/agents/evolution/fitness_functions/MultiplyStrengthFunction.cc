#include "MultiplyStrengthFunction.h"
#include "AtomDBUtils.h"
#include "Logger.h"
#include "Utils.h"

using namespace std;
using namespace fitness_functions;

string MultiplyStrengthFunction::VARIABLE_NAME = "strength";

float MultiplyStrengthFunction::eval(shared_ptr<QueryAnswer> query_answer, shared_ptr<Keychain> keychain) {
    float strength = 1.0;

    LOG_DEBUG("Evaluating strength for " << query_answer->to_string());

    for (const auto& handle : query_answer->get_handles_vector()) {
        shared_ptr<Atom> atom;
        if (this->key_sensitive_atomdb == nullptr) {
            atom = this->atomdb->get_atom(handle);
        } else {
            atom = this->key_sensitive_atomdb->get_atom(handle, keychain);
        }
        LOG_DEBUG("Evaluating strength for handle: " << handle);
        LOG_DEBUG("MeTTa expression: " << AtomDBUtils::handle_to_metta(handle, keychain));
        LOG_DEBUG("Atom document: " << atom->to_string());
        strength *= atom->custom_attributes.get_or<double>(VARIABLE_NAME, 1.0);
    }
    LOG_DEBUG("Computed strength: " << strength);
    return strength;
}
