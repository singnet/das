#include "InferenceToyFunction.h"

#include "AtomDBSingleton.h"
#include "Logger.h"
#include "Utils.h"

using namespace std;
using namespace atomdb;
using namespace fitness_functions;

string InferenceToyFunction::STRENGTH_TAG = "strength";

InferenceToyFunction::InferenceToyFunction() { db = AtomDBSingleton::get_instance(); }

float InferenceToyFunction::eval(shared_ptr<QueryAnswer> query_answer) {
    LOG_DEBUG("Computing strength for: " << query_answer->to_string());

    string atom_handle = query_answer->get(0);
    auto atom = db->get_atom(atom_handle);
    LOG_DEBUG("Evaluation link: " << atom->to_string());
    float strength = atom->custom_attributes.get_or<double>(STRENGTH_TAG, 1.0);

    for (unsigned int path_index = 0; path_index < 2; path_index++) {
        LOG_DEBUG("Path index: " << path_index);
        vector<string>& path = query_answer->get_path_vector(path_index);
        for (string& handle : path) {
            atom = db->get_atom(handle);
            LOG_DEBUG("Link: " << atom->to_string());
            strength *= atom->custom_attributes.get_or<double>(STRENGTH_TAG, 1.0);
        }
    }
    LOG_DEBUG("Computed strength: " << strength);
    return strength;
}
