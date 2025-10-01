#include "Context.h"

#define LOG_LEVEL INFO_LEVEL
#include <fstream>
#include <iostream>

#include "AtomDBSingleton.h"
#include "AttentionBrokerClient.h"
#include "Hasher.h"
#include "Logger.h"
#include "PatternMatchingQueryProxy.h"
#include "ServiceBusSingleton.h"

using namespace atom_space;
using namespace atoms;
using namespace atomdb;
using namespace attention_broker;
using namespace query_engine;
using namespace service_bus;
using namespace commons;

// -------------------------------------------------------------------------------------------------
// Public methods

Context::Context(const string& name, Atom& atom_key) {
    // Toplevel Atom
    init(name);
    if (!this->cached) {
        string ID = RedisMongoDB::MONGODB_FIELD_NAME[MONGODB_FIELD::ID];
        string TARGETS = RedisMongoDB::MONGODB_FIELD_NAME[MONGODB_FIELD::TARGETS];
        string TYPE = RedisMongoDB::MONGODB_FIELD_NAME[MONGODB_FIELD::NAMED_TYPE];
        auto documents = AtomDBSingleton::get_instance()->get_matching_atoms(true, atom_key);
        vector<string> determiners;
        for (const auto& document : documents) {
            if (document->contains(TARGETS)) {  // if is link
                determiners.clear();
                string handle = string(document->get(ID));
                determiners.push_back(handle);
                this->to_stimulate[handle] = 1;
                unsigned int arity = document->get_size(TARGETS);
                for (unsigned int i = 1; i < arity; i++) {
                    determiners.push_back(string(document->get(TARGETS, i)));
                }
                this->determiner_request.push_back(determiners);
            }
        }
    }
    update_attention_broker();
}

Context::Context(const string& name,
                 const vector<string>& query,
                 const vector<pair<QueryAnswerElement, QueryAnswerElement>>& determiner_schema,
                 vector<QueryAnswerElement>& stimulus_schema) {
    // Query
    init(name);
    if (!this->cached) {
        add_determiners(query, determiner_schema, stimulus_schema);
        cache_write();
    }
    update_attention_broker();
}

void Context::add_determiners(
    const vector<string>& query,
    const vector<pair<QueryAnswerElement, QueryAnswerElement>>& determiner_schema,
    vector<QueryAnswerElement>& stimulus_schema) {
    this->determiner_request.clear();
    this->to_stimulate.clear();

    // Query
    auto proxy = make_shared<PatternMatchingQueryProxy>(query, this->key);
    proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = true;
    proxy->parameters[BaseQueryProxy::ATTENTION_UPDATE_FLAG] = false;
    proxy->parameters[BaseQueryProxy::USE_LINK_TEMPLATE_CACHE] = false;
    proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = false;
    proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] = false;

    ServiceBusSingleton::get_instance()->issue_bus_command(proxy);
#if LOG_LEVEL >= DEBUG_LEVEL
    unsigned int count_query_answer = 0;
#endif
    string source, target;
    while (!proxy->finished()) {
        shared_ptr<QueryAnswer> answer = proxy->pop();
        if (answer != NULL) {
            for (auto pair : determiner_schema) {
                source = answer->get(pair.first);
                target = answer->get(pair.second);
                if ((source != "") && (target != "")) {
                    this->determiner_request.push_back({source, target});
                }
            }
            for (auto element : stimulus_schema) {
                target = answer->get(element);
                if (target != "") {
                    this->to_stimulate[answer->get(element)] = 1;
                }
            }
#if LOG_LEVEL >= DEBUG_LEVEL
            if (!(++count_query_answer % 100000)) {
                LOG_DEBUG("Number of QueryAnswer objects processed so far: " +
                          std::to_string(count_query_answer));
            }
#endif
        } else {
            Utils::sleep();
        }
    }
}

Context::~Context() {
    // TODO delete context in AttentionBroker
}

void Context::init(const string& name) {
    this->name = name;
    this->key = Hasher::context_handle(name);
    this->cache_file_name = CACHE_FILE_NAME_PREFIX + this->key + ".txt";
    this->cached = cache_read();
}

void Context::cache_write() {
    LOG_INFO("Caching computed context into file: " + this->cache_file_name);

    ofstream file(this->cache_file_name);

    if (!file.is_open()) {
        LOG_ERROR("Couldn't open file " + this->cache_file_name + " for writing");
        LOG_INFO("Context info wont be cached");
        return;
    }

    file << this->to_stimulate.size() << endl;
    for (auto pair : this->to_stimulate) {
        file << pair.first << endl;
    }
    file << this->determiner_request.size() << endl;
    for (auto sub_vector : this->determiner_request) {
        file << sub_vector.size() << endl;
        for (string handle : sub_vector) {
            file << handle << endl;
        }
    }

    file.close();
}

static inline void read_line(ifstream& file, string& line) {
    if (!getline(file, line)) {
        Utils::error("Error reading a line from cache file");
    }
}

bool Context::cache_read() {
    ifstream file(this->cache_file_name);
    if (file.is_open()) {
        LOG_INFO("Reading Context info from cache file: " + this->cache_file_name);
    } else {
        LOG_INFO("Couldn't open file " + this->cache_file_name + " for reading");
        LOG_INFO("No cached info will be used");
        return false;
    }

    string line;
    read_line(file, line);
    int size = Utils::string_to_int(line);
    for (int i = 0; i < size; i++) {
        read_line(file, line);
        this->to_stimulate[line] = 1;
    }
    read_line(file, line);
    size = Utils::string_to_int(line);
    this->determiner_request.reserve(size);
    vector<string> sub_vector;
    for (int i = 0; i < size; i++) {
        read_line(file, line);
        int sub_vector_size = Utils::string_to_int(line);
        sub_vector.clear();
        sub_vector.reserve(sub_vector_size);
        for (int j = 0; j < sub_vector_size; j++) {
            read_line(file, line);
            sub_vector.push_back(line);
        }
        this->determiner_request.push_back(sub_vector);
    }

    file.close();
    return true;
}

void Context::update_attention_broker() {
    AttentionBrokerClient::set_determiners(this->determiner_request, this->key);
    if (this->to_stimulate.size() > 0) {
        AttentionBrokerClient::stimulate(this->to_stimulate, this->key);
    }
    this->to_stimulate.clear();
    this->determiner_request.clear();
}

const string& Context::get_name() { return this->name; }

const string& Context::get_key() { return this->key; }
