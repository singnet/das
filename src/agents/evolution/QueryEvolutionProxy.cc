#include "QueryEvolutionProxy.h"

#include "ServiceBus.h"

#define LOG_LEVEL INFO_LEVEL
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

QueryEvolutionProxy::QueryEvolutionProxy(const vector<string>& tokens, const string& context)
    : BaseProxy() {
    // constructor typically used in requestor
    lock_guard<mutex> semaphore(this->api_mutex);
    init();
    this->command = ServiceBus::QUERY_EVOLUTION;
    this->args.insert(this->args.end(), tokens.begin(), tokens.end());
}

void QueryEvolutionProxy::set_default_query_parameters() { this->unique_assignment_flag = false; }

void QueryEvolutionProxy::init() {
    this->answer_count = 0;
    set_default_query_parameters();
}

void QueryEvolutionProxy::pack_custom_args() {
    vector<string> custom_args = {this->context, to_string(this->unique_assignment_flag)};
    this->args.insert(this->args.begin(), custom_args.begin(), custom_args.end());
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

// ---------------------------------------------------------------------------------------------
// Virtual superclass API and the piggyback methods called by it

bool QueryEvolutionProxy::from_remote_peer(const string& command, const vector<string>& args) {
    LOG_DEBUG("Proxy command: <" << command << "> from " << this->peer_id() << " received in "
                                 << this->my_id());
    if (!BaseProxy::from_remote_peer(command, args)) {
        if (command == ANSWER_BUNDLE) {
            answer_bundle(args);
            return true;
        } else {
            Utils::error("Invalid proxy command: <" + command + ">");
        }
    }
    return false;
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
