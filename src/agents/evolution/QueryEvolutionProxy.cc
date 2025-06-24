#include "QueryEvolutionProxy.h"

#include "FitnessFunctionRegistry.h"
#include "ServiceBus.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace evolution;

// -------------------------------------------------------------------------------------------------
// Constructors, destructors and initialization

string QueryEvolutionProxy::POPULATION_SIZE = "population_size";
string QueryEvolutionProxy::MAX_GENERATIONS = "max_generations";
string QueryEvolutionProxy::ELITISM_RATE = "elitism_rate";
string QueryEvolutionProxy::SELECTION_RATE = "selection_rate";

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
    set_default_query_parameters();
    set_fitness_function_tag(fitness_function);
    this->command = ServiceBus::QUERY_EVOLUTION;
    this->best_reported_fitness = -1;
    this->num_generations = 0;
}

void QueryEvolutionProxy::set_default_query_parameters() {
    this->parameters[POPULATION_SIZE] = (unsigned int) 1000;
    this->parameters[MAX_GENERATIONS] = (unsigned int) 100;
    this->parameters[ELITISM_RATE] = (double) 0.01;
    this->parameters[SELECTION_RATE] = (double) 0.1;
}

string QueryEvolutionProxy::to_string() {
    lock_guard<mutex> semaphore(this->api_mutex);
    string answer = "{";
    answer += BaseQueryProxy::to_string();
    answer += " fitness_function: " + this->fitness_function_tag;
    answer += "}";
    return answer;
}

QueryEvolutionProxy::~QueryEvolutionProxy() {}

// -------------------------------------------------------------------------------------------------
// Client-side API

void QueryEvolutionProxy::pack_command_line_args() { tokenize(this->args); }

void QueryEvolutionProxy::tokenize(vector<string>& output) {
    lock_guard<mutex> semaphore(this->api_mutex);
    output.insert(output.begin(), this->fitness_function_tag);
    BaseQueryProxy::tokenize(output);
}

// -------------------------------------------------------------------------------------------------
// Server-side API

void QueryEvolutionProxy::untokenize(vector<string>& tokens) {
    BaseQueryProxy::untokenize(tokens);
    set_fitness_function_tag(tokens[0]);
    tokens.erase(tokens.begin(), tokens.begin() + 1);
}

float QueryEvolutionProxy::compute_fitness(shared_ptr<QueryAnswer> answer) {
    if (this->fitness_function_tag == "") {
        Utils::error("Invalid empty fitness function tag");
        return 0;
    } else {
        return this->fitness_function_object->eval(answer);
    }
}

bool QueryEvolutionProxy::stop_criteria_met() {
    return (this->num_generations >= this->parameters.get<unsigned int>(MAX_GENERATIONS));
}

void QueryEvolutionProxy::new_population_sampled(
    vector<std::pair<shared_ptr<QueryAnswer>, float>>& population) {
    if (population[0].second > best_reported_fitness) {
        for (int i = population.size() - 1; i >= 0; i--) {
            if (population[i].second > this->best_reported_fitness) {
                push(population[i].first);
                this->best_reported_fitness = population[i].second;
            }
        }
    }
    this->num_generations++;
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
