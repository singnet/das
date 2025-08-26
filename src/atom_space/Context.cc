#include "Context.h"

#include "AttentionBrokerClient.h"
#include "AtomDBSingleton.h"
#include "UntypedVariable.h"
#include "Hasher.h"

using namespace atom_space;
using namespace atoms;
using namespace atomdb;
using namespace attention_broker;
using namespace commons;

// -------------------------------------------------------------------------------------------------
// Public methods

Context::Context(const string& name) {
    this->name = name;
    this->key = Hasher::context_handle(name);
    set_determiners();
}

Context::~Context() {
    // TODO delete context in AttentionBroker
}

void Context::set_determiners() {
    UntypedVariable v1("v1");
    string ID = RedisMongoDB::MONGODB_FIELD_NAME[MONGODB_FIELD::ID];
    string TARGETS = RedisMongoDB::MONGODB_FIELD_NAME[MONGODB_FIELD::TARGETS];
    string TYPE = RedisMongoDB::MONGODB_FIELD_NAME[MONGODB_FIELD::NAMED_TYPE];
    auto documents = AtomDBSingleton::get_instance()->get_matching_atoms(true, v1);
    vector<string> request;
    vector<vector<string>> requests;
    for (const auto& document : documents) {
        if (document == nullptr) {
            cout << "XXXXXXXXXXXXXXXXXXXXXXXXX document is NULL" << endl;
        } else {
            cout << "XXXXXXXXXXXXXXXXXXXXXXXXX document type: " << string(document->get(TYPE)) << endl;
        }
        if (document->contains(TARGETS)) { // if is link
            request.clear();
            request.push_back(string(document->get(ID)));
            unsigned int arity = document->get_size(TARGETS);
            for (unsigned int i = 0; i < arity; i++) {
                request.push_back(string(document->get(TARGETS, i)));
                //cout << "XXXXXXXXXXXXXXXXXXXXXXXXX " << string(document->get(ID)) << " -> " << string(document->get(TARGETS, i)) << endl;
            }
            requests.push_back(request);
        } else {
        }
    }
    AttentionBrokerClient::set_determiners(requests, this->key);
}

const string& Context::get_name() { return name; }

const string& Context::get_key() { return key; }
