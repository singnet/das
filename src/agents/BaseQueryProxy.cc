#include "BaseQueryProxy.h"

#include "ServiceBus.h"

#define LOG_LEVEL DEBUG_LEVEL
#include "Logger.h"

using namespace agents;

// -------------------------------------------------------------------------------------------------
// Constructors, destructors and initialization

string BaseQueryProxy::ABORT = "abort";
string BaseQueryProxy::ANSWER_BUNDLE = "answer_bundle";
string BaseQueryProxy::FINISHED = "finished";

BaseQueryProxy::BaseQueryProxy() {
    // constructor typically used in processor
    lock_guard<mutex> semaphore(this->api_mutex);
    init();
}

BaseQueryProxy::BaseQueryProxy(const vector<string>& tokens, const string& context) : BaseProxy() {
    // constructor typically used in requestor
    lock_guard<mutex> semaphore(this->api_mutex);
    init();
    this->context = context;
    this->args.insert(this->args.end(), tokens.begin(), tokens.end());
}

void BaseQueryProxy::init() {
    this->answer_count = 0;
    this->unique_assignment_flag = false;
    this->attention_update_flag = false;
}

BaseQueryProxy::~BaseQueryProxy() {}

// -------------------------------------------------------------------------------------------------
// Client-side API

shared_ptr<QueryAnswer> BaseQueryProxy::pop() {
    lock_guard<mutex> semaphore(this->api_mutex);
    if (this->is_aborting()) {
        return shared_ptr<QueryAnswer>(NULL);
    } else {
        return shared_ptr<QueryAnswer>((QueryAnswer*) this->answer_queue.dequeue());
    }
}

unsigned int BaseQueryProxy::get_count() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->answer_count;
}

void BaseQueryProxy::set_count(unsigned int count) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->answer_count = count;
}

// -------------------------------------------------------------------------------------------------
// Server-side API

const string& BaseQueryProxy::get_context() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->context;
}

void BaseQueryProxy::set_context(const string& context) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->context = context;
}

const vector<string>& BaseQueryProxy::get_query_tokens() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->query_tokens;
}

string BaseQueryProxy::to_string() {
    string answer = "{";
    answer += "context: " + this->get_context();
    answer += " unique_assignment: " + string(this->get_unique_assignment_flag() ? "true" : "false");
    answer += " attention_update_flag: " + string(attention_update_flag ? "true" : "false");
    answer += "}";
    return answer;
}

// -------------------------------------------------------------------------------------------------
// Query parameters getters and setters

bool BaseQueryProxy::get_unique_assignment_flag() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->unique_assignment_flag;
}

void BaseQueryProxy::set_unique_assignment_flag(bool flag) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->unique_assignment_flag = flag;
}

bool BaseQueryProxy::get_attention_update_flag() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->attention_update_flag;
}

void BaseQueryProxy::set_attention_update_flag(bool flag) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->attention_update_flag = flag;
}

// ---------------------------------------------------------------------------------------------
// Virtual superclass API and the piggyback methods called by it

void BaseQueryProxy::raise_error(const string& error_message, unsigned int error_code) {
    string error = "Exception thrown in command processor.";
    if (error_code > 0) {
        error += " Error code: " + std::to_string(error_code);
    }
    error += "\n";
    error += error_message;
    this->error_flag = true;
    this->error_code = error_code;
    this->error_message = error;
    LOG_ERROR(error);
    command_finished({});
}

bool BaseQueryProxy::from_remote_peer(const string& command, const vector<string>& args) {
    LOG_DEBUG("Proxy command: <" << command << "> from " << this->peer_id() << " received in "
                                 << this->my_id());
    if (BaseProxy::from_remote_peer(command, args)) {
        return true;
    } else {
        if (command == ANSWER_BUNDLE) {
            answer_bundle(args);
        } else {
            return false;
        }
        return true;
    }
}

void BaseQueryProxy::answer_bundle(const vector<string>& args) {
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
