#include "ContextBrokerProxy.h"

#include "AtomDBSingleton.h"
#include "Hasher.h"
#include "RedisMongoDB.h"
#include "ServiceBus.h"
#include "ServiceBusSingleton.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace context_broker;
using namespace atomdb;
using namespace query_engine;
using namespace service_bus;
using namespace commons;

// -------------------------------------------------------------------------------------------------
// Static constants

// Cache file name prefix
const string CACHE_FILE_NAME_PREFIX = "_CONTEXT_CACHE_";

// -------------------------------------------------------------------------------------------------
// Constructors, destructors and initialization

ContextBrokerProxy::ContextBrokerProxy(bool use_cache) {
    // Empty constructor typically used on server side
    this->command = ServiceBus::CONTEXT_MANAGER;
    this->use_cache = use_cache;
}

ContextBrokerProxy::ContextBrokerProxy(
    const string& name,
    const vector<string>& query,
    const vector<pair<QueryAnswerElement, QueryAnswerElement>>& determiner_schema,
    const vector<QueryAnswerElement>& stimulus_schema,
    bool use_cache,
    float rent_rate,
    float spreading_rate_lowerbound,
    float spreading_rate_upperbound)
    : BaseQueryProxy(query, name) {
    // Constructor for query-based context
    this->command = ServiceBus::CONTEXT_MANAGER;
    this->determiner_schema = determiner_schema;
    this->stimulus_schema = stimulus_schema;
    this->rent_rate = rent_rate;
    this->spreading_rate_lowerbound = spreading_rate_lowerbound;
    this->spreading_rate_upperbound = spreading_rate_upperbound;
    init(name, use_cache);
}

ContextBrokerProxy::~ContextBrokerProxy() {}

void ContextBrokerProxy::from_atomdb(const string& atom_handle) {
    lock_guard<mutex> semaphore(this->api_mutex);
    if (!this->use_cache) {
        string ID = RedisMongoDB::MONGODB_FIELD_NAME[MONGODB_FIELD::ID];
        string TARGETS = RedisMongoDB::MONGODB_FIELD_NAME[MONGODB_FIELD::TARGETS];
        string TYPE = RedisMongoDB::MONGODB_FIELD_NAME[MONGODB_FIELD::NAMED_TYPE];

        // Get the Atom object from the handle
        auto atom = AtomDBSingleton::get_instance()->get_atom(atom_handle);
        if (atom) {
            auto documents = AtomDBSingleton::get_instance()->get_matching_atoms(true, *atom);
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
    }
}

// -------------------------------------------------------------------------------------------------
// BaseQueryProxy overrides

void ContextBrokerProxy::tokenize(vector<string>& output) {
    lock_guard<mutex> semaphore(this->api_mutex);
    // Add context-specific data to the beginning (in reverse order)
    output.insert(output.begin(), this->name);
    output.insert(output.begin(), this->key);
    output.insert(output.begin(), this->atom_handle);
    output.insert(output.begin(), this->use_cache ? "1" : "0");

    // AttentionBroker parameters
    output.insert(output.begin(), std::to_string(this->rent_rate));
    output.insert(output.begin(), std::to_string(this->spreading_rate_lowerbound));
    output.insert(output.begin(), std::to_string(this->spreading_rate_upperbound));

    // Add determiner_schema
    for (int i = this->determiner_schema.size() - 1; i >= 0; i--) {
        output.insert(output.begin(), this->determiner_schema[i].second.to_string());
        output.insert(output.begin(), this->determiner_schema[i].first.to_string());
    }
    output.insert(output.begin(), std::to_string(this->determiner_schema.size()));

    // Add stimulus_schema
    for (auto& element : this->stimulus_schema) {
        output.insert(output.begin(), element.to_string());
    }
    output.insert(output.begin(), std::to_string(this->stimulus_schema.size()));

    BaseQueryProxy::tokenize(output);
}

void ContextBrokerProxy::untokenize(vector<string>& tokens) {
    lock_guard<mutex> semaphore(this->api_mutex);
    BaseQueryProxy::untokenize(tokens);

    // Extract context-specific data from the beginning (in reverse order of tokenize)
    // stimulus_schema (last inserted, so first to extract)
    int stimulus_size = std::stoi(tokens[0]);
    tokens.erase(tokens.begin());
    this->stimulus_schema.clear();
    for (int i = 0; i < stimulus_size; i++) {
        QueryAnswerElement element(tokens[0]);
        this->stimulus_schema.push_back(element);
        tokens.erase(tokens.begin());
    }

    // determiner_schema (second to last inserted)
    int determiner_size = std::stoi(tokens[0]);
    tokens.erase(tokens.begin());
    this->determiner_schema.clear();
    for (int i = 0; i < determiner_size; i++) {
        auto first = QueryAnswerElement::from_string(tokens[0]);
        tokens.erase(tokens.begin());
        auto second = QueryAnswerElement::from_string(tokens[0]);
        tokens.erase(tokens.begin());
        this->determiner_schema.push_back(make_pair(first, second));
    }

    // AttentionBroker parameters
    this->spreading_rate_upperbound = std::stof(tokens[0]);
    tokens.erase(tokens.begin());
    this->spreading_rate_lowerbound = std::stof(tokens[0]);
    tokens.erase(tokens.begin());
    this->rent_rate = std::stof(tokens[0]);
    tokens.erase(tokens.begin());

    // context-specific data (first inserted, so last to extract)
    this->use_cache = (tokens[0] == "1");
    tokens.erase(tokens.begin());
    this->atom_handle = tokens[0];
    tokens.erase(tokens.begin());
    this->key = tokens[0];
    tokens.erase(tokens.begin());
    this->name = tokens[0];
    tokens.erase(tokens.begin());
    this->cache_file_name = CACHE_FILE_NAME_PREFIX + this->key + ".txt";
}

string ContextBrokerProxy::to_string() {
    lock_guard<mutex> semaphore(this->api_mutex);
    string answer = "{ContextBrokerProxy: ";
    answer += "name: " + this->name + ", ";
    answer += "key: " + this->key + ", ";
    answer += "atom_handle: " + this->atom_handle + ", ";
    answer += "use_cache: " + string(this->use_cache ? "true" : "false") + ", ";
    answer += "AB { rent_rate: " + std::to_string(this->rent_rate) + ", ";
    answer += "spreading_rate_lowerbound: " + std::to_string(this->spreading_rate_lowerbound) + ", ";
    answer += "spreading_rate_upperbound: " + std::to_string(this->spreading_rate_upperbound) + " } ";
    answer += "BaseQueryProxy: ";
    answer += BaseQueryProxy::to_string();
    answer += "}";
    return answer;
}

void ContextBrokerProxy::pack_command_line_args() { tokenize(this->args); }

// -------------------------------------------------------------------------------------------------
// Private methods

void ContextBrokerProxy::init(const string& name, bool use_cache) {
    this->name = name;
    this->key = Hasher::context_handle(name);
    this->cache_file_name = CACHE_FILE_NAME_PREFIX + this->key + ".txt";
    this->use_cache = use_cache;
}

// -------------------------------------------------------------------------------------------------
// Public accessor methods

const string& ContextBrokerProxy::get_name() { return this->name; }

const string& ContextBrokerProxy::get_key() { return this->key; }

const string& ContextBrokerProxy::get_atom_handle() { return this->atom_handle; }

const vector<pair<QueryAnswerElement, QueryAnswerElement>>& ContextBrokerProxy::get_determiner_schema() {
    return this->determiner_schema;
}

const vector<QueryAnswerElement>& ContextBrokerProxy::get_stimulus_schema() {
    return this->stimulus_schema;
}

vector<vector<string>>& ContextBrokerProxy::get_determiner_request() { return this->determiner_request; }

map<string, unsigned int>& ContextBrokerProxy::get_to_stimulate() { return this->to_stimulate; }

void ContextBrokerProxy::clear_to_stimulate() { this->to_stimulate.clear(); }

void ContextBrokerProxy::clear_determiner_request() { this->determiner_request.clear(); }

const string& ContextBrokerProxy::get_cache_file_name() { return this->cache_file_name; }

bool ContextBrokerProxy::get_use_cache() { return this->use_cache; }

float ContextBrokerProxy::get_rent_rate() { return this->rent_rate; }

float ContextBrokerProxy::get_spreading_rate_lowerbound() { return this->spreading_rate_lowerbound; }

float ContextBrokerProxy::get_spreading_rate_upperbound() { return this->spreading_rate_upperbound; }
