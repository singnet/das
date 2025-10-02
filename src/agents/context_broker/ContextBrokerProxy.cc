#include "ContextBrokerProxy.h"

#include "AtomDBSingleton.h"
#include "AttentionBrokerServer.h"
#include "Hasher.h"
#include "RedisMongoDB.h"
#include "ServiceBus.h"
#include "ServiceBusSingleton.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace context_broker;
using namespace atomdb;
using namespace attention_broker;
using namespace query_engine;
using namespace service_bus;
using namespace commons;

// -------------------------------------------------------------------------------------------------
// Static constants

// Cache file name prefix
const string CACHE_FILE_NAME_PREFIX = "_CONTEXT_CACHE_";

// Proxy Commands
string ContextBrokerProxy::ATTENTION_BROKER_SET_PARAMETERS = "attention_broker_set_parameters";
string ContextBrokerProxy::ATTENTION_BROKER_SET_PARAMETERS_FINISHED =
    "attention_broker_set_parameters_finished";
string ContextBrokerProxy::CONTEXT_CREATED = "context_created";
string ContextBrokerProxy::SHUTDOWN = "shutdown";

// Properties
string ContextBrokerProxy::USE_CACHE = "use_cache";
string ContextBrokerProxy::ENFORCE_CACHE_RECREATION = "enforce_cache_recreation";
string ContextBrokerProxy::INITIAL_RENT_RATE = "initial_rent_rate";
string ContextBrokerProxy::INITIAL_SPREADING_RATE_LOWERBOUND = "initial_spreading_rate_lowerbound";
string ContextBrokerProxy::INITIAL_SPREADING_RATE_UPPERBOUND = "initial_spreading_rate_upperbound";

// Default values for AttentionBrokerClient::set_parameters()
double ContextBrokerProxy::DEFAULT_RENT_RATE = AttentionBrokerServer::RENT_RATE;
double ContextBrokerProxy::DEFAULT_SPREADING_RATE_LOWERBOUND =
    AttentionBrokerServer::SPREADING_RATE_LOWERBOUND;
double ContextBrokerProxy::DEFAULT_SPREADING_RATE_UPPERBOUND =
    AttentionBrokerServer::SPREADING_RATE_UPPERBOUND;

// -------------------------------------------------------------------------------------------------
// Constructors, destructors and initialization

ContextBrokerProxy::ContextBrokerProxy() {
    // Empty constructor typically used on server side
    this->command = ServiceBus::CONTEXT;
    set_default_query_parameters();
    this->update_attention_broker_parameters = false;
    this->ongoing_attention_broker_set_parameters = false;
    this->context_created = false;
    this->keep_alive = true;
}

ContextBrokerProxy::ContextBrokerProxy(
    const string& name,
    const vector<string>& query,
    const vector<pair<QueryAnswerElement, QueryAnswerElement>>& determiner_schema,
    const vector<QueryAnswerElement>& stimulus_schema)
    : BaseQueryProxy(query, name) {
    // Constructor for query-based context
    this->command = ServiceBus::CONTEXT;
    this->determiner_schema = determiner_schema;
    this->stimulus_schema = stimulus_schema;
    set_default_query_parameters();
    init(name);
    this->update_attention_broker_parameters = false;
    this->ongoing_attention_broker_set_parameters = false;
    this->context_created = false;
    this->keep_alive = true;
}

ContextBrokerProxy::~ContextBrokerProxy() { to_remote_peer(SHUTDOWN, {}); }

// -------------------------------------------------------------------------------------------------
// BaseQueryProxy overrides

void ContextBrokerProxy::tokenize(vector<string>& output) {
    lock_guard<mutex> semaphore(this->api_mutex);
    // Add context-specific data to the beginning (in reverse order)
    output.insert(output.begin(), this->name);
    output.insert(output.begin(), this->key);

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
        auto element = QueryAnswerElement::from_string(tokens[0]);
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

    // context-specific data (first inserted, so last to extract)
    this->key = tokens[0];
    tokens.erase(tokens.begin());
    this->name = tokens[0];
    tokens.erase(tokens.begin());
    this->cache_file_name = CACHE_FILE_NAME_PREFIX + this->key + ".txt";
}

string ContextBrokerProxy::to_string() {
    lock_guard<mutex> semaphore(this->api_mutex);

    bool use_cache = this->get_use_cache();
    bool enforce_cache_recreation = this->get_enforce_cache_recreation();

    string answer = "{ContextBrokerProxy: ";
    answer += "name: " + this->name + ", ";
    answer += "key: " + this->key + ", ";
    answer += "use_cache: " + string(use_cache ? "true" : "false") + ", ";
    answer += "enforce_cache_recreation: " + string(enforce_cache_recreation ? "true" : "false") + ", ";
    answer += "AB { rent_rate: " + std::to_string(this->rent_rate) + ", ";
    answer += "spreading_rate_lowerbound: " + std::to_string(this->spreading_rate_lowerbound) + ", ";
    answer += "spreading_rate_upperbound: " + std::to_string(this->spreading_rate_upperbound) + " } ";
    answer += "BaseQueryProxy: ";
    answer += BaseQueryProxy::to_string();
    answer += "}";
    return answer;
}

void ContextBrokerProxy::pack_command_line_args() { tokenize(this->args); }

bool ContextBrokerProxy::is_context_created() { return this->context_created; }

bool ContextBrokerProxy::running() { return this->keep_alive; }

// -------------------------------------------------------------------------------------------------
// Private methods

void ContextBrokerProxy::init(const string& name) {
    this->name = name;
    this->key = Hasher::context_handle(name);
    this->cache_file_name = CACHE_FILE_NAME_PREFIX + this->key + ".txt";
}

void ContextBrokerProxy::set_default_query_parameters() {
    this->parameters[USE_CACHE] = (bool) true;
    this->parameters[ENFORCE_CACHE_RECREATION] = (bool) false;
    this->parameters[INITIAL_RENT_RATE] = (double) DEFAULT_RENT_RATE;
    this->parameters[INITIAL_SPREADING_RATE_LOWERBOUND] = (double) DEFAULT_SPREADING_RATE_LOWERBOUND;
    this->parameters[INITIAL_SPREADING_RATE_UPPERBOUND] = (double) DEFAULT_SPREADING_RATE_UPPERBOUND;
}

// ---------------------------------------------------------------------------------------------
// Virtual superclass API and the piggyback methods called by it

bool ContextBrokerProxy::from_remote_peer(const string& command, const vector<string>& args) {
    LOG_DEBUG("Proxy command: <" << command << "> from " << this->peer_id() << " received in "
                                 << this->my_id());
    if (BaseQueryProxy::from_remote_peer(command, args)) {
        return true;
    } else if (command == ATTENTION_BROKER_SET_PARAMETERS) {
        this->rent_rate = Utils::string_to_float(args[0]);
        this->spreading_rate_lowerbound = Utils::string_to_float(args[1]);
        this->spreading_rate_upperbound = Utils::string_to_float(args[2]);
        this->update_attention_broker_parameters = true;
        return true;
    } else if (command == CONTEXT_CREATED) {
        this->context_created = true;
        return true;
    } else if (command == ATTENTION_BROKER_SET_PARAMETERS_FINISHED) {
        this->ongoing_attention_broker_set_parameters = false;
        return true;
    } else if (command == SHUTDOWN) {
        this->keep_alive = false;
        return true;
    } else {
        Utils::error("Invalid ContextBrokerProxy command: <" + command + ">");
        return false;
    }
}

void ContextBrokerProxy::attention_broker_set_parameters(double rent_rate,
                                                         double spreading_rate_lowerbound,
                                                         double spreading_rate_upperbound) {
    vector<string> args;
    args.push_back(std::to_string(rent_rate));
    args.push_back(std::to_string(spreading_rate_lowerbound));
    args.push_back(std::to_string(spreading_rate_upperbound));
    this->ongoing_attention_broker_set_parameters = true;
    to_remote_peer(ATTENTION_BROKER_SET_PARAMETERS, args);
}

void ContextBrokerProxy::shutdown() { to_remote_peer(SHUTDOWN, {}); }

// -------------------------------------------------------------------------------------------------
// Public accessor methods

const string& ContextBrokerProxy::get_name() { return this->name; }

const string& ContextBrokerProxy::get_key() { return this->key; }

const vector<pair<QueryAnswerElement, QueryAnswerElement>>& ContextBrokerProxy::get_determiner_schema() {
    return this->determiner_schema;
}

const vector<QueryAnswerElement>& ContextBrokerProxy::get_stimulus_schema() {
    return this->stimulus_schema;
}

const string& ContextBrokerProxy::get_cache_file_name() { return this->cache_file_name; }

bool ContextBrokerProxy::get_use_cache() { return this->parameters.get<bool>(USE_CACHE); }

bool ContextBrokerProxy::get_enforce_cache_recreation() {
    return this->parameters.get<bool>(ENFORCE_CACHE_RECREATION);
}

double ContextBrokerProxy::get_initial_rent_rate() {
    return this->parameters.get<double>(INITIAL_RENT_RATE);
}

double ContextBrokerProxy::get_initial_spreading_rate_lowerbound() {
    return this->parameters.get<double>(INITIAL_SPREADING_RATE_LOWERBOUND);
}

double ContextBrokerProxy::get_initial_spreading_rate_upperbound() {
    return this->parameters.get<double>(INITIAL_SPREADING_RATE_UPPERBOUND);
}

bool ContextBrokerProxy::attention_broker_set_parameters_finished() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return !this->ongoing_attention_broker_set_parameters;
}
