#include "QueryEvolutionProxy.h"

#include "FitnessFunctionRegistry.h"
#include "ServiceBus.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace evolution;

// -------------------------------------------------------------------------------------------------
// Constructors, destructors and initialization

string QueryEvolutionProxy::EVAL_FITNESS = "eval_fitness";
string QueryEvolutionProxy::EVAL_FITNESS_RESPONSE = "eval_fitness_response";

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
                                         const vector<vector<string>>& correlation_queries,
                                         const vector<map<string, QueryAnswerElement>>& correlation_replacements,
                                         const vector<pair<QueryAnswerElement, QueryAnswerElement>>& correlation_mappings,
                                         const string& context,
                                         const string& fitness_function_tag,
                                         shared_ptr<FitnessFunction> fitness_function)
    : BaseQueryProxy(tokens, context) {
    // constructor typically used in requestor
    init();
    set_default_query_parameters();
    this->fitness_function_object = fitness_function;
    set_fitness_function_tag(fitness_function_tag);
    this->correlation_queries = correlation_queries;
    this->correlation_replacements = correlation_replacements;
    this->correlation_mappings = correlation_mappings;
}

void QueryEvolutionProxy::init() {
    this->command = ServiceBus::QUERY_EVOLUTION;
    this->best_reported_fitness = -1;
    this->num_generations = 0;
    this->fitness_function_object = shared_ptr<FitnessFunction>(nullptr);
    this->ongoing_remote_fitness_evaluation = false;
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
    answer += ", correlation_queries: [";
    for (auto query : this->correlation_queries) {
        answer += "[";
        for (string token : query) {
            answer += token + ", ";
        }
        if (query.size() > 0) {
            answer.pop_back();
            answer.pop_back();
        }
        answer += "], ";
    }
    if (this->correlation_queries.size() > 0) {
        answer.pop_back();
        answer.pop_back();
    }
    answer += "], correlation_replacements: [";
    for (auto mapping : this->correlation_replacements) {
        answer += "{";
        for (auto pair : mapping) {
            answer += "{" + pair.first + ", " + pair.second.to_string() + "}, ";
        }
        if (mapping.size() > 0) {
            answer.pop_back();
            answer.pop_back();
        }
        answer += "}, ";
    }
    if (this->correlation_replacements.size() > 0) {
        answer.pop_back();
        answer.pop_back();
    }
    answer += "], correlation_mappings: [";
    for (auto pair : this->correlation_mappings) {
        answer += "(" + pair.first.to_string() + ", " + pair.second.to_string() + "), ";
    }
    if (this->correlation_mappings.size() > 0) {
        answer.pop_back();
        answer.pop_back();
    }
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
    for (int i = this->correlation_mappings.size() - 1; i >=0; i--) {
        output.insert(output.begin(), this->correlation_mappings[i].second.to_string());
        output.insert(output.begin(), this->correlation_mappings[i].first.to_string());
    }
    output.insert(output.begin(), std::to_string(this->correlation_mappings.size()));
    for (int i = this->correlation_replacements.size() - 1; i >=0; i--) {
        for (auto pair : this->correlation_replacements[i]) {
            output.insert(output.begin(), pair.second.to_string());
            output.insert(output.begin(), pair.first);
        }
        output.insert(output.begin(), std::to_string(this->correlation_replacements[i].size()));
    }
    output.insert(output.begin(), std::to_string(this->correlation_replacements.size()));
    for (int i = this->correlation_queries.size() - 1; i >=0; i--) {
        output.insert(output.begin(), this->correlation_queries[i].begin(), this->correlation_queries[i].end());
        output.insert(output.begin(), std::to_string(this->correlation_queries[i].size()));
    }
    output.insert(output.begin(), std::to_string(this->correlation_queries.size()));
    output.insert(output.begin(), this->fitness_function_tag);
    BaseQueryProxy::tokenize(output);
}

// -------------------------------------------------------------------------------------------------
// Server-side API

void QueryEvolutionProxy::untokenize(vector<string>& tokens) {
    BaseQueryProxy::untokenize(tokens);
    set_fitness_function_tag(tokens[0]);
    tokens.erase(tokens.begin(), tokens.begin() + 1);

    unsigned int num_correlation_queries = std::stoi(tokens[0]);
    tokens.erase(tokens.begin(), tokens.begin() + 1);
    vector<string> query;
    for (unsigned int i = 0; i < num_correlation_queries; i++) {
        unsigned int query_size = std::stoi(tokens[0]);
        query.clear();
        query.insert(query.begin(),
                     tokens.begin() + 1,
                     tokens.begin() + 1 + query_size);
        this->correlation_queries.push_back(query);
        tokens.erase(tokens.begin(), tokens.begin() + 1 + query_size);
    }

    unsigned int num_correlation_replacements = std::stoi(tokens[0]);
    tokens.erase(tokens.begin(), tokens.begin() + 1);
    map<string, QueryAnswerElement> mapping;
    for (unsigned int i = 0; i < num_correlation_replacements; i++) {
        unsigned int mapping_size = std::stoi(tokens[0]);
        mapping.clear();
        for (unsigned int j = 0; j < mapping_size; j++) {
            string key = tokens[2 * j + 1];
            string value = tokens[2 * j + 2];
            if (value[0] == '_') {
                QueryAnswerElement element((unsigned int) Utils::string_to_int(value.substr(1, value.size() - 1)));
                mapping[key] = element;
            } else {
                QueryAnswerElement element(value.substr(1, value.size() - 1));
                mapping[key] = element;
            }
        }
        this->correlation_replacements.push_back(mapping);
        tokens.erase(tokens.begin(), tokens.begin() + 1 + (2 * mapping_size));
    }

    unsigned int num_correlation_mappings = std::stoi(tokens[0]);
    for (unsigned int i = 0; i < num_correlation_replacements; i++) {
        string source = tokens[2 * i + 1];
        string target = tokens[2 * i + 2];
        QueryAnswerElement source_element;
        QueryAnswerElement target_element;
        if (source[0] == '_') {
            QueryAnswerElement element((unsigned int) Utils::string_to_int(source.substr(1, source.size() - 1)));
            source_element = element;
        } else {
            QueryAnswerElement element(source.substr(1, source.size() - 1));
            source_element = element;
        }
        if (target[0] == '_') {
            QueryAnswerElement element((unsigned int) Utils::string_to_int(target.substr(1, target.size() - 1)));
            target_element = element;
        } else {
            QueryAnswerElement element(target.substr(1, target.size() - 1));
            target_element = element;
        }
        this->correlation_mappings.push_back({source_element, target_element});
    }
    tokens.erase(tokens.begin(), tokens.begin() + 1 + (2 * num_correlation_mappings));
}

float QueryEvolutionProxy::compute_fitness(shared_ptr<QueryAnswer> answer) {
    if (this->fitness_function_tag == "") {
        Utils::error("Invalid empty fitness function tag");
        return 0;
    } else if (this->fitness_function_object == nullptr) {
        if (this->fitness_function_tag == FitnessFunctionRegistry::REMOTE_FUNCTION) {
            Utils::error("Invalid call to remote function");
        } else {
            Utils::error("Fitness function is not set up");
        }
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
        if (population[0].second > this->best_reported_fitness) {
            for (int i = population.size() - 1; i >= 0; i--) {
                if (population[i].second >= this->best_reported_fitness) {
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
        if (tag != FitnessFunctionRegistry::REMOTE_FUNCTION) {
            this->fitness_function_object = FitnessFunctionRegistry::function(tag);
        }
    }
}

const vector<vector<string>>& QueryEvolutionProxy::get_correlation_queries() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->correlation_queries;
}

const vector<map<string, QueryAnswerElement>>& QueryEvolutionProxy::get_correlation_replacements() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->correlation_replacements;
}

const vector<pair<QueryAnswerElement, QueryAnswerElement>>& QueryEvolutionProxy::get_correlation_mappings() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->correlation_mappings;
}

bool QueryEvolutionProxy::is_fitness_function_remote() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return (this->fitness_function_object == nullptr) &&
           (this->fitness_function_tag == FitnessFunctionRegistry::REMOTE_FUNCTION);
}

void QueryEvolutionProxy::remote_fitness_evaluation(const vector<string>& answer_bundle) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->ongoing_remote_fitness_evaluation = true;
    this->remote_fitness_evaluation_result.clear();
    to_remote_peer(EVAL_FITNESS, answer_bundle);
}

bool QueryEvolutionProxy::remote_fitness_evaluation_finished() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return !this->ongoing_remote_fitness_evaluation;
}

vector<float> QueryEvolutionProxy::get_remotely_evaluated_fitness() {
    lock_guard<mutex> semaphore(this->api_mutex);
    // This method doesn't return a reference to avoid concurrency hazard
    return this->remote_fitness_evaluation_result;
}

// ---------------------------------------------------------------------------------------------
// Virtual superclass API and the piggyback methods called by it

bool QueryEvolutionProxy::from_remote_peer(const string& command, const vector<string>& args) {
    LOG_DEBUG("Proxy command: <" << command << "> from " << this->peer_id() << " received in "
                                 << this->my_id());
    if (BaseQueryProxy::from_remote_peer(command, args)) {
        return true;
    } else if (command == EVAL_FITNESS) {
        eval_fitness(args);
        return true;
    } else if (command == EVAL_FITNESS_RESPONSE) {
        eval_fitness_response(args);
        return true;
    } else {
        Utils::error("Invalid QueryEvolutionProxy command: <" + command + ">");
        return false;
    }
}

void QueryEvolutionProxy::eval_fitness(const vector<string>& args) {
    lock_guard<mutex> semaphore(this->api_mutex);
    if (!this->is_aborting()) {
        if (args.size() == 0) {
            Utils::error("Invalid empty query answer bundle");
        } else {
            vector<string> fitness_bundle;
            for (auto tokens : args) {
                shared_ptr<QueryAnswer> query_answer = make_shared<QueryAnswer>();
                query_answer->untokenize(tokens);
                float fitness = compute_fitness(query_answer);
                fitness_bundle.push_back(std::to_string(fitness));
            }
            to_remote_peer(EVAL_FITNESS_RESPONSE, fitness_bundle);
        }
    }
}

void QueryEvolutionProxy::eval_fitness_response(const vector<string>& args) {
    lock_guard<mutex> semaphore(this->api_mutex);
    if (!this->is_aborting()) {
        if (args.size() == 0) {
            Utils::error("Invalid empty fitness value bundle");
        } else {
            for (auto value_str : args) {
                float fitness = Utils::string_to_float(value_str);
                this->remote_fitness_evaluation_result.push_back(fitness);
            }
            this->ongoing_remote_fitness_evaluation = false;
        }
    }
}
