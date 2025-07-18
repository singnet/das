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
string QueryEvolutionProxy::TOTAL_ATTENTION_TOKENS = "total_attention_tokens";

QueryEvolutionProxy::QueryEvolutionProxy() {
    // constructor typically used in processor
    lock_guard<mutex> semaphore(this->api_mutex);
    init();
    set_default_query_parameters();
}

QueryEvolutionProxy::QueryEvolutionProxy(const vector<string>& tokens,
                                         const vector<string>& correlation_tokens,
                                         const vector<string>& correlation_variables,
                                         const string& fitness_function,
                                         const string& context)
    : BaseQueryProxy(tokens, context) {
    // constructor typically used in requestor
    init();
    set_default_query_parameters();
    set_fitness_function_tag(fitness_function);
    this->correlation_tokens = correlation_tokens;
    this->correlation_variables = correlation_variables;
}

void QueryEvolutionProxy::init() {
    this->command = ServiceBus::QUERY_EVOLUTION;
    this->best_reported_fitness = -1;
    this->num_generations = 0;
}

void QueryEvolutionProxy::set_default_query_parameters() {
    this->parameters[POPULATION_SIZE] = (unsigned int) 1000;
    this->parameters[MAX_GENERATIONS] = (unsigned int) 100;
    this->parameters[ELITISM_RATE] = (double) 0.01;
    this->parameters[SELECTION_RATE] = (double) 0.1;
    this->parameters[TOTAL_ATTENTION_TOKENS] = (unsigned int) 100;
}

string QueryEvolutionProxy::to_string() {
    lock_guard<mutex> semaphore(this->api_mutex);
    string answer = "{BaseQueryProxy: ";
    answer += BaseQueryProxy::to_string();
    answer += ", fitness_function: " + this->fitness_function_tag;
    answer += ", correlation_tokens: [";
    for (auto token : this->correlation_tokens) {
        answer += token + ", ";
    }
    answer.pop_back();
    answer.pop_back();
    answer += "], ";
    answer += "correlation_variables: [";
    for (auto token : this->correlation_variables) {
        answer += token + ", ";
    }
    answer.pop_back();
    answer.pop_back();
    answer += "]";
    answer += "}";
    return answer;
}

QueryEvolutionProxy::~QueryEvolutionProxy() {}

// -------------------------------------------------------------------------------------------------
// Client-side API

void QueryEvolutionProxy::pack_command_line_args() { tokenize(this->args); }

void QueryEvolutionProxy::tokenize(vector<string>& output) {
    lock_guard<mutex> semaphore(this->api_mutex);
    output.insert(
        output.begin(), this->correlation_variables.begin(), this->correlation_variables.end());
    output.insert(output.begin(), std::to_string(this->correlation_variables.size()));
    output.insert(output.begin(), this->correlation_tokens.begin(), this->correlation_tokens.end());
    output.insert(output.begin(), std::to_string(this->correlation_tokens.size()));
    output.insert(output.begin(), this->fitness_function_tag);
    BaseQueryProxy::tokenize(output);
}

// -------------------------------------------------------------------------------------------------
// Server-side API

void QueryEvolutionProxy::untokenize(vector<string>& tokens) {
    BaseQueryProxy::untokenize(tokens);
    set_fitness_function_tag(tokens[0]);
    tokens.erase(tokens.begin(), tokens.begin() + 1);

    unsigned int num_correlation_tokens = std::stoi(tokens[0]);
    this->correlation_tokens.insert(this->correlation_tokens.begin(),
                                    tokens.begin() + 1,
                                    tokens.begin() + 1 + num_correlation_tokens);
    tokens.erase(tokens.begin(), tokens.begin() + 1 + num_correlation_tokens);

    unsigned int num_correlation_variables = std::stoi(tokens[0]);
    this->correlation_variables.insert(this->correlation_variables.begin(),
                                       tokens.begin() + 1,
                                       tokens.begin() + 1 + num_correlation_variables);
    tokens.erase(tokens.begin(), tokens.begin() + 1 + num_correlation_variables);
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
    if (population.size() > 0) {
        if (population[0].second > best_reported_fitness) {
            for (int i = population.size() - 1; i >= 0; i--) {
                if (population[i].second > this->best_reported_fitness) {
                    push(population[i].first);
                    this->best_reported_fitness = population[i].second;
                }
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

const vector<string>& QueryEvolutionProxy::get_correlation_tokens() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->correlation_tokens;
}

const vector<string>& QueryEvolutionProxy::get_correlation_variables() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->correlation_variables;
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
