#include "PatternMatchingQueryProxy.h"

#include "ServiceBus.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace query_engine;

// -------------------------------------------------------------------------------------------------
// Constructors, destructors and initialization

string PatternMatchingQueryProxy::COUNT = "count";

PatternMatchingQueryProxy::PatternMatchingQueryProxy() {
    // constructor typically used in processor
    lock_guard<mutex> semaphore(this->api_mutex);
    init();
}

PatternMatchingQueryProxy::PatternMatchingQueryProxy(const vector<string>& tokens, const string& context)
    : BaseQueryProxy(tokens, context) {
    // constructor typically used in requestor
    lock_guard<mutex> semaphore(this->api_mutex);
    init();
    this->command = ServiceBus::PATTERN_MATCHING_QUERY;
}

void PatternMatchingQueryProxy::set_default_query_parameters() {
    this->positive_importance_flag = false;
    this->count_flag = false;
}

void PatternMatchingQueryProxy::init() {
    this->error_flag = false;
    set_default_query_parameters();
}

PatternMatchingQueryProxy::~PatternMatchingQueryProxy() {}

// -------------------------------------------------------------------------------------------------
// Client-side API

shared_ptr<QueryAnswer> PatternMatchingQueryProxy::pop() {
    lock_guard<mutex> semaphore(this->api_mutex);
    if (this->count_flag) {
        Utils::error("Can't pop QueryAnswers from count_only queries.");
        return shared_ptr<QueryAnswer>(NULL);
    } else {
        return BaseQueryProxy::pop();
    }
}

// -------------------------------------------------------------------------------------------------
// Server-side API

void PatternMatchingQueryProxy::pack_custom_args() {
    vector<string> custom_args = {this->get_context(),
                                  std::to_string(this->get_unique_assignment_flag()),
                                  std::to_string(this->positive_importance_flag),
                                  std::to_string(this->get_attention_update_flag()),
                                  std::to_string(this->count_flag)};
    this->args.insert(this->args.begin(), custom_args.begin(), custom_args.end());
}

string PatternMatchingQueryProxy::to_string() {
    string answer = "{";
    answer += BaseQueryProxy::to_string();
    answer += " count_flag: " + string(this->get_count_flag() ? "true" : "false");
    answer +=
        " positive_importance_flag: " + string(this->get_positive_importance_flag() ? "true" : "false");
    answer += "}";
    return answer;
}

// -------------------------------------------------------------------------------------------------
// Query parameters getters and setters

bool PatternMatchingQueryProxy::get_count_flag() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->count_flag;
}

void PatternMatchingQueryProxy::set_count_flag(bool flag) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->count_flag = flag;
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
// Virtual superclass API and the piggyback methods called by it

bool PatternMatchingQueryProxy::from_remote_peer(const string& command, const vector<string>& args) {
    LOG_DEBUG("Proxy command: <" << command << "> from " << this->peer_id() << " received in "
                                 << this->my_id());

    if (BaseQueryProxy::from_remote_peer(command, args)) {
        return true;
    } else {
        if (command == COUNT) {
            count_answer(args);
            return true;
        } else {
            Utils::error("Invalid proxy command: <" + command + ">");
            return false;
        }
    }
}

void PatternMatchingQueryProxy::count_answer(const vector<string>& args) {
    lock_guard<mutex> semaphore(this->api_mutex);
    if (!this->is_aborting()) {
        if (args.size() != 1) {
            Utils::error("Invalid args for count command");
        }
        if (!this->count_flag) {
            Utils::error("Invalid count command. Query is not count_only.");
        } else {
            this->set_count(stoi(args[0]));
        }
    }
}
