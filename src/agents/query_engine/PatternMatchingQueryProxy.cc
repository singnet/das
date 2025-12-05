#include "PatternMatchingQueryProxy.h"

#include "ServiceBus.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace query_engine;

// -------------------------------------------------------------------------------------------------
// Constructors, destructors and initialization

string PatternMatchingQueryProxy::COUNT = "count";

string PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG = "positive_importance_flag";
string PatternMatchingQueryProxy::UNIQUE_VALUE_FLAG = "unique_value_flag";
string PatternMatchingQueryProxy::COUNT_FLAG = "count_flag";

PatternMatchingQueryProxy::PatternMatchingQueryProxy() {
    // constructor typically used in processor
    lock_guard<mutex> semaphore(this->api_mutex);
    set_default_parameters();
}

PatternMatchingQueryProxy::PatternMatchingQueryProxy(const vector<string>& tokens, const string& context)
    : BaseQueryProxy(tokens, context) {
    // constructor typically used in requestor
    lock_guard<mutex> semaphore(this->api_mutex);
    set_default_parameters();
    this->command = ServiceBus::PATTERN_MATCHING_QUERY;
}

void PatternMatchingQueryProxy::set_default_parameters() {
    this->parameters[POSITIVE_IMPORTANCE_FLAG] = false;
    this->parameters[UNIQUE_VALUE_FLAG] = false;
    this->parameters[COUNT_FLAG] = false;
}

PatternMatchingQueryProxy::~PatternMatchingQueryProxy() {}

// -------------------------------------------------------------------------------------------------
// Client-side API

shared_ptr<QueryAnswer> PatternMatchingQueryProxy::pop() {
    lock_guard<mutex> semaphore(this->api_mutex);
    if (this->parameters.get<bool>(COUNT_FLAG)) {
        Utils::error("Can't pop QueryAnswers from count_only queries.");
        return shared_ptr<QueryAnswer>(NULL);
    } else {
        return BaseQueryProxy::pop();
    }
}

// -------------------------------------------------------------------------------------------------
// Server-side API

void PatternMatchingQueryProxy::pack_command_line_args() { tokenize(this->args); }

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
        if (!this->parameters.get<bool>(COUNT_FLAG)) {
            Utils::error("Invalid count command. Query is not count_only.");
        } else {
            this->set_count(stoi(args[0]));
        }
    }
}
