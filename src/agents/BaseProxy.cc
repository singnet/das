#include "BaseProxy.h"

#include "ServiceBus.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace agents;

// -------------------------------------------------------------------------------------------------
// Constructors, destructors and initialization

string BaseProxy::ABORT = "abort";
string BaseProxy::FINISHED = "finished";

BaseProxy::BaseProxy() {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->command_finished_flag = false;
    this->abort_flag = false;
    this->error_flag = false;
}

BaseProxy::~BaseProxy() {}

// -------------------------------------------------------------------------------------------------
// Client-side API

bool BaseProxy::finished() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->abort_flag || this->command_finished_flag;
}

void BaseProxy::abort() {
    lock_guard<mutex> semaphore(this->api_mutex);
    Utils::error("Method not implemented");
    if (!this->command_finished_flag) {
        to_remote_peer(ABORT, {});
    }
    this->abort_flag = true;
}

void BaseProxy::tokenize(vector<string>& output) {
    vector<string> parameters_tokens = this->parameters.tokenize();
    parameters_tokens.insert(parameters_tokens.begin(), std::to_string(parameters_tokens.size()));
    output.insert(output.begin(), parameters_tokens.begin(), parameters_tokens.end());
}

// -------------------------------------------------------------------------------------------------
// Server-side API

void BaseProxy::untokenize(vector<string>& tokens) {
    unsigned int num_property_tokens = std::stoi(tokens[0]);
    if (num_property_tokens > 0) {
        vector<string> properties_tokens;
        properties_tokens.insert(
            properties_tokens.begin(), tokens.begin() + 1, tokens.begin() + 1 + num_property_tokens);
        this->parameters.untokenize(properties_tokens);
        tokens.erase(tokens.begin(), tokens.begin() + 1 + num_property_tokens);
    }
}

bool BaseProxy::is_aborting() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->abort_flag;
}

string BaseProxy::to_string() {
    string answer = "{";
    answer += "parameters: " + this->parameters.to_string();
    answer += "}";
    return answer;
}

// ---------------------------------------------------------------------------------------------
// Virtual superclass API and the piggyback methods called by it

void BaseProxy::raise_error(const string& error_message, unsigned int error_code) {
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

bool BaseProxy::from_remote_peer(const string& command, const vector<string>& args) {
    LOG_DEBUG("Proxy command: <" << command << "> from " << this->peer_id() << " received in "
                                 << this->my_id());
    if (!BusCommandProxy::from_remote_peer(command, args)) {
        if (command == FINISHED) {
            command_finished(args);
        } else if (command == ABORT) {
            abort(args);
        } else {
            return false;
        }
    }
    return true;
}

void BaseProxy::command_finished(const vector<string>& args) {
    lock_guard<mutex> semaphore(this->api_mutex);
    if (!this->abort_flag) {
        this->command_finished_flag = true;
    }
}

void BaseProxy::abort(const vector<string>& args) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->abort_flag = true;
}
