#include "QueryEvolutionProxy.h"

#include "FitnessFunctionRegistry.h"
#include "ServiceBus.h"

#define LOG_LEVEL DEBUG_LEVEL
#include "Logger.h"

using namespace evolution;

// -------------------------------------------------------------------------------------------------
// Constructors, destructors and initialization

QueryEvolutionProxy::QueryEvolutionProxy() {
    // constructor typically used in processor
    lock_guard<mutex> semaphore(this->api_mutex);
    set_default_query_parameters();
}

QueryEvolutionProxy::QueryEvolutionProxy(const vector<string>& tokens,
                                         const string& fitness_function,
                                         const string& context)
    : BaseQueryProxy(tokens, context) {
    // constructor typically used in requestor
    lock_guard<mutex> semaphore(this->api_mutex);
    set_default_query_parameters();
    this->fitness_function_tag = fitness_function;
    this->command = ServiceBus::QUERY_EVOLUTION;
}

void QueryEvolutionProxy::set_default_query_parameters() {
    this->fitness_function_tag = "";
    this->population_size = 1000;
}

void QueryEvolutionProxy::pack_custom_args() {
    vector<string> custom_args = {
        this->get_context(),
        std::to_string(this->get_unique_assignment_flag()),
        this->fitness_function_tag,
        std::to_string(this->population_size),
    };
    this->args.insert(this->args.begin(), custom_args.begin(), custom_args.end());
}

string QueryEvolutionProxy::to_string() {
    string answer = "{";
    answer += BaseQueryProxy::to_string();
    answer += " fitness_function: " + this->get_fitness_function_tag();
    answer += " population_size: " + std::to_string(this->get_population_size());
    answer += "}";
    return answer;
}

QueryEvolutionProxy::~QueryEvolutionProxy() {}

// -------------------------------------------------------------------------------------------------
// Server-side API

float QueryEvolutionProxy::compute_fitness(shared_ptr<QueryAnswer> answer) {
    if (this->fitness_function_tag == "") {
        Utils::error("Invalid empty fitness function tag");
        return 0;
    } else {
        return this->fitness_function_object->eval(answer);
    }
}

// -------------------------------------------------------------------------------------------------
// Query parameters getters and setters

const string& QueryEvolutionProxy::get_fitness_function_tag() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->fitness_function_tag;
}

void QueryEvolutionProxy::set_fitness_function_tag(const string& tag) {
    lock_guard<mutex> semaphore(this->api_mutex);
    if ((this->fitness_function_tag != "") && (tag != this->fitness_function_tag)) {
        Utils::error("Invalid reset of fitness function: " + this->fitness_function_tag + " --> " + tag);
    } else {
        if (tag == "") {
            Utils::error("Invalid empty fitness function tag");
        }
        this->fitness_function_tag = tag;
        this->fitness_function_object = FitnessFunctionRegistry::function(tag);
    }
}

unsigned int QueryEvolutionProxy::get_population_size() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->population_size;
}

void QueryEvolutionProxy::set_population_size(unsigned int population_size) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->population_size = population_size;
}

// ---------------------------------------------------------------------------------------------
// Virtual superclass API and the piggyback methods called by it

bool QueryEvolutionProxy::from_remote_peer(const string& command, const vector<string>& args) {
    LOG_DEBUG("Proxy command: <" << command << "> from " << this->peer_id() << " received in "
                                 << this->my_id());
    if (BaseQueryProxy::from_remote_peer(command, args)) {
        return true;
    } else {
        Utils::error("Invalid QueryEvolutionProxy command: <" + command + ">");
        return false;
    }
}
