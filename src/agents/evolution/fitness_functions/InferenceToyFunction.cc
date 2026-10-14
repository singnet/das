#include "InferenceToyFunction.h"

#include "Logger.h"
#include "Utils.h"

using namespace std;
using namespace fitness_functions;

string InferenceToyFunction::STRENGTH_TAG = "strength";

float InferenceToyFunction::eval(shared_ptr<QueryAnswer> query_answer, shared_ptr<Keychain> keychain) {
    LOG_DEBUG("Computing strength for: " << query_answer->to_string());

    string atom_handle = query_answer->get(0);
    shared_ptr<Atom> atom;
    if (this->key_sensitive_atomdb == nullptr) {
        atom = this->atomdb->get_atom(atom_handle);
    } else {
        atom = this->key_sensitive_atomdb->get_atom(atom_handle, keychain);
    }
    LOG_DEBUG("Evaluation link: " << atom->to_string());
    float strength = atom->custom_attributes.get_or<double>(STRENGTH_TAG, 1.0);

    unsigned int num_paths = query_answer->get_paths_size();
    for (unsigned int path_index = 0; path_index < num_paths; path_index++) {
        LOG_DEBUG("Path index: " << path_index);
        vector<string>& path = query_answer->get_path_vector(path_index);
        for (string& handle : path) {
            if (this->key_sensitive_atomdb == nullptr) {
                atom = this->atomdb->get_atom(handle);
            } else {
                atom = this->key_sensitive_atomdb->get_atom(handle, keychain);
            }
            LOG_DEBUG("Link: " << atom->to_string());
            strength *= atom->custom_attributes.get_or<double>(STRENGTH_TAG, 1.0);
        }
    }
    LOG_DEBUG("Computed strength: " << strength);
    return strength;
}
