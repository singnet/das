#include "PatternMatchingQueryProxy.h"

#include "ServiceBus.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace query_engine;

// -------------------------------------------------------------------------------------------------
// Constructors, destructors and initialization

string PatternMatchingQueryProxy::ABORT = "abort";
string PatternMatchingQueryProxy::ANSWER_BUNDLE = "answer_bundle";
string PatternMatchingQueryProxy::COUNT = "count";
string PatternMatchingQueryProxy::FINISHED = "finished";

PatternMatchingQueryProxy::PatternMatchingQueryProxy() {
    // constructor typically used in processor
    lock_guard<mutex> semaphore(this->api_mutex);
    init();
}

PatternMatchingQueryProxy::PatternMatchingQueryProxy(const vector<string>& tokens, const string& context)
    : BusCommandProxy() {
    // constructor typically used in requestor
    lock_guard<mutex> semaphore(this->api_mutex);
    init();
    this->command = ServiceBus::PATTERN_MATCHING_QUERY;
    this->args.insert(this->args.end(), tokens.begin(), tokens.end());
}

void PatternMatchingQueryProxy::set_default_query_parameters() {
    this->unique_assignment_flag = false;
    this->positive_importance_flag = false;
    this->update_attention_broker = false;
    this->count_flag = false;
}

void PatternMatchingQueryProxy::init() {
    this->answer_flow_finished = false;
    this->abort_flag = false;
    this->answer_count = 0;
    set_default_query_parameters();
}

void PatternMatchingQueryProxy::pack_custom_args() {
    vector<string> custom_args = {this->context,
                                  to_string(this->unique_assignment_flag),
                                  to_string(this->positive_importance_flag),
                                  to_string(this->update_attention_broker),
                                  to_string(this->count_flag)};
    this->args.insert(this->args.begin(), custom_args.begin(), custom_args.end());
}

PatternMatchingQueryProxy::~PatternMatchingQueryProxy() {}

// -------------------------------------------------------------------------------------------------
// Client-side API

bool PatternMatchingQueryProxy::finished() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->abort_flag ||
           (this->answer_flow_finished && (this->count_flag || (this->answer_queue.size() == 0)));
}

shared_ptr<QueryAnswer> PatternMatchingQueryProxy::pop() {
    lock_guard<mutex> semaphore(this->api_mutex);
    if (this->count_flag) {
        Utils::error("Can't pop QueryAnswers from count_only queries.");
    }
    if (this->abort_flag) {
        return shared_ptr<QueryAnswer>(NULL);
    } else {
        return shared_ptr<QueryAnswer>((QueryAnswer*) this->answer_queue.dequeue());
    }
}

unsigned int PatternMatchingQueryProxy::get_count() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->answer_count;
}

void PatternMatchingQueryProxy::abort() {
    lock_guard<mutex> semaphore(this->api_mutex);
    Utils::error("Method not implemented");
    if (!this->answer_flow_finished) {
        to_remote_peer(ABORT, {});
    }
    this->abort_flag = true;
}

// -------------------------------------------------------------------------------------------------
// Server-side API

bool PatternMatchingQueryProxy::is_aborting() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->abort_flag;
}

const string& PatternMatchingQueryProxy::get_context() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->context;
}

void PatternMatchingQueryProxy::set_context(const string& context) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->context = context;
}

const vector<string>& PatternMatchingQueryProxy::get_query_tokens() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->query_tokens;
}

// -------------------------------------------------------------------------------------------------
// Query parameters getters and setters

bool PatternMatchingQueryProxy::get_attention_update_flag() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->update_attention_broker;
}

void PatternMatchingQueryProxy::set_attention_update_flag(bool flag) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->update_attention_broker = flag;
}

bool PatternMatchingQueryProxy::get_count_flag() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->count_flag;
}

void PatternMatchingQueryProxy::set_count_flag(bool flag) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->count_flag = flag;
}

bool PatternMatchingQueryProxy::get_unique_assignment_flag() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->unique_assignment_flag;
}

void PatternMatchingQueryProxy::set_unique_assignment_flag(bool flag) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->unique_assignment_flag = flag;
}

bool PatternMatchingQueryProxy::get_positive_importance_flag() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->positive_importance_flag;
}

void PatternMatchingQueryProxy::set_positive_importance_flag(bool flag) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->positive_importance_flag = flag;
}

// ---------------------------------------------------------------------------------------------
// Virtual superclass API from_remote_peer() and the piggyback methods called by it

void PatternMatchingQueryProxy::from_remote_peer(const string& command, const vector<string>& args) {
    LOG_DEBUG("Proxy command: <" << command << "> from " << this->peer_id() << " received in "
                                 << this->my_id());
    if (command == ANSWER_BUNDLE) {
        answer_bundle(args);
    } else if (command == COUNT) {
        count_answer(args);
    } else if (command == FINISHED) {
        query_answers_finished(args);
    } else if (command == ABORT) {
        abort(args);
    } else {
        Utils::error("Invalid proxy command: <" + command + ">");
    }
}

void PatternMatchingQueryProxy::answer_bundle(const vector<string>& args) {
    lock_guard<mutex> semaphore(this->api_mutex);
    if (!this->abort_flag) {
        if (args.size() == 0) {
            Utils::error("Invalid empty query answer");
        }
        if (this->count_flag) {
            Utils::error("Invalid answer_bundle command. Query is count_only.");
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

void PatternMatchingQueryProxy::count_answer(const vector<string>& args) {
    lock_guard<mutex> semaphore(this->api_mutex);
    if (!this->abort_flag) {
        if (args.size() != 1) {
            Utils::error("Invalid args for count command");
        }
        if (!this->count_flag) {
            Utils::error("Invalid count command. Query is not count_only.");
        } else {
            this->answer_count = stoi(args[0]);
        }
    }
}

void PatternMatchingQueryProxy::query_answers_finished(const vector<string>& args) {
    lock_guard<mutex> semaphore(this->api_mutex);
    if (!this->abort_flag) {
        this->answer_flow_finished = true;
    }
}

void PatternMatchingQueryProxy::abort(const vector<string>& args) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->abort_flag = true;
}
