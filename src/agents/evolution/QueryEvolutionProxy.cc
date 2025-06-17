#include "QueryEvolutionProxy.h"

#include "FitnessFunctionRegistry.h"
#include "ServiceBus.h"

#define LOG_LEVEL DEBUG_LEVEL
#include "Logger.h"

using namespace evolution;

// -------------------------------------------------------------------------------------------------
// Constructors, destructors and initialization

string QueryEvolutionProxy::ANSWER_BUNDLE = "answer_bundle";

QueryEvolutionProxy::QueryEvolutionProxy() {
    // constructor typically used in processor
    lock_guard<mutex> semaphore(this->api_mutex);
    init();
}

QueryEvolutionProxy::QueryEvolutionProxy(const vector<string>& tokens,
                                         const string& fitness_function,
                                         const string& context)
    : BaseProxy() {
    // constructor typically used in requestor
    lock_guard<mutex> semaphore(this->api_mutex);
    init();
    this->fitness_function_tag = fitness_function;
    this->context = context;
    this->command = ServiceBus::QUERY_EVOLUTION;
    this->args.insert(this->args.end(), tokens.begin(), tokens.end());
}

void QueryEvolutionProxy::set_default_query_parameters() {
    this->unique_assignment_flag = false;
    this->fitness_function_tag = "";
    this->population_size = 1000;
}

void QueryEvolutionProxy::init() {
    this->answer_count = 0;
    set_default_query_parameters();
}

void QueryEvolutionProxy::pack_custom_args() {
    vector<string> custom_args = {
        this->context, 
        std::to_string(this->unique_assignment_flag), 
        this->fitness_function_tag,
        std::to_string(this->population_size),
    };
    this->args.insert(this->args.begin(), custom_args.begin(), custom_args.end());
}

string QueryEvolutionProxy::to_string() {
    string answer = "{";
    answer += "context: " + this->get_context();
    answer += " unique_assignment: " + string(this->get_unique_assignment_flag() ? "true" : "false");
    answer += " fitness_function: " + this->get_fitness_function_tag();
    answer += " population_size: " + this->get_population_size();
    answer += "}";
    return answer;
}

QueryEvolutionProxy::~QueryEvolutionProxy() {}

// -------------------------------------------------------------------------------------------------
// Client-side API

shared_ptr<QueryAnswer> QueryEvolutionProxy::pop() {
    lock_guard<mutex> semaphore(this->api_mutex);
    if (this->is_aborting()) {
        return shared_ptr<QueryAnswer>(NULL);
    } else {
        return shared_ptr<QueryAnswer>((QueryAnswer*) this->answer_queue.dequeue());
    }
}

unsigned int QueryEvolutionProxy::get_count() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->answer_count;
}

// -------------------------------------------------------------------------------------------------
// Server-side API

const string& QueryEvolutionProxy::get_context() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->context;
}

void QueryEvolutionProxy::set_context(const string& context) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->context = context;
}

const vector<string>& QueryEvolutionProxy::get_query_tokens() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->query_tokens;
}

float QueryEvolutionProxy::compute_fitness(shared_ptr<QueryAnswer> answer) {
    if (this->fitness_function_tag == "") {
        Utils::error("Invalid empty fitness function tag");
    } else {
        return this->fitness_function_object->eval(answer);
    }
}

// -------------------------------------------------------------------------------------------------
// Query parameters getters and setters

bool QueryEvolutionProxy::get_unique_assignment_flag() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->unique_assignment_flag;
}

void QueryEvolutionProxy::set_unique_assignment_flag(bool flag) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->unique_assignment_flag = flag;
}

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
    if (!BaseProxy::from_remote_peer(command, args)) {
        if (command == ANSWER_BUNDLE) {
            answer_bundle(args);
        } else {
            Utils::error("Invalid QueryEvolutionProxy command: <" + command + ">");
            return false;
        }
    }
    return true;
}

void QueryEvolutionProxy::answer_bundle(const vector<string>& args) {
    lock_guard<mutex> semaphore(this->api_mutex);
    if (!this->is_aborting()) {
        if (args.size() == 0) {
            Utils::error("Invalid empty query answer");
        } else {
            for (auto tokens : args) {
                QueryAnswer* query_answer = new QueryAnswer();
                query_answer->untokenize(tokens);
                this->answer_queue.enqueue((void*) query_answer);
                this->answer_count++;
            }
        }
    }
}
