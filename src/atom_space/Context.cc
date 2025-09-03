#include "Context.h"

#include "AtomDBSingleton.h"
#include "AttentionBrokerClient.h"
#include "Hasher.h"

using namespace atom_space;
using namespace atoms;
using namespace atomdb;
using namespace attention_broker;
using namespace commons;

// -------------------------------------------------------------------------------------------------
// Public methods

Context::Context(const string& name, Atom& atom_key) {
    this->name = name;
    this->key = Hasher::context_handle(name);
    set_determiners(atom_key);
}

Context::~Context() {
    // TODO delete context in AttentionBroker
}

void Context::set_determiners(Atom& atom_key) {
    string ID = RedisMongoDB::MONGODB_FIELD_NAME[MONGODB_FIELD::ID];
    string TARGETS = RedisMongoDB::MONGODB_FIELD_NAME[MONGODB_FIELD::TARGETS];
    string TYPE = RedisMongoDB::MONGODB_FIELD_NAME[MONGODB_FIELD::NAMED_TYPE];
    auto documents = AtomDBSingleton::get_instance()->get_matching_atoms(true, atom_key);
    map<string, unsigned int> to_stimulate;
    vector<string> determiners;
    vector<vector<string>> determiner_request;
    for (const auto& document : documents) {
        if (document->contains(TARGETS)) {  // if is link
            determiners.clear();
            string handle = string(document->get(ID));
            determiners.push_back(handle);
            to_stimulate[handle] = 1;
            unsigned int arity = document->get_size(TARGETS);
            for (unsigned int i = 0; i < arity; i++) {
                determiners.push_back(string(document->get(TARGETS, i)));
            }
            determiner_request.push_back(determiners);
        }
    }
    AttentionBrokerClient::set_determiners(determiner_request, this->key);
    //AttentionBrokerClient::stimulate(to_stimulate, this->key); // XXXXXXXXXXXXXXXXX
}

const string& Context::get_name() { return this->name; }

const string& Context::get_key() { return this->key; }
