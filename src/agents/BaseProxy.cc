#include "BaseProxy.h"

#include "Logger.h"
#include "ServiceBus.h"
#include "SystemParametersSingleton.h"
#include "Utils.h"

using namespace agents;

// -------------------------------------------------------------------------------------------------
// Constructors, destructors and initialization

string BaseProxy::ABORT = "abort";
string BaseProxy::FINISHED = "finished";
string BaseProxy::ALLOW_CYCLE_START = "allow_cycle_start";

string BaseProxy::ORCHESTRATION_SCHEMA = "orchestration_schema";

BaseProxy::BaseProxy() {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->command_finished_flag = false;
    this->abort_flag = false;
    this->error_flag = false;
    this->cycle_start_allowed_flag = false;
    this->parameters = SystemParametersSingleton::get_instance()->get_base_proxy_params();

    set_orchestration_schema(
        (ORCHESTRATION_SCHEMA_TYPE) this->parameters.get<unsigned int>(ORCHESTRATION_SCHEMA));
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
    // RAISE_ERROR("Method not implemented");
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
    unsigned int num_property_tokens =
        Utils::string_to_int(tokens[0]);  // safe conversion, should always be a number
    if (num_property_tokens > 0) {
        vector<string> properties_tokens;
        properties_tokens.insert(
            properties_tokens.begin(), tokens.begin() + 1, tokens.begin() + 1 + num_property_tokens);
        this->parameters.untokenize(properties_tokens);
        set_orchestration_schema(
            (ORCHESTRATION_SCHEMA_TYPE) this->parameters.get<unsigned int>(ORCHESTRATION_SCHEMA));
        tokens.erase(tokens.begin(), tokens.begin() + 1 + num_property_tokens);
    } else {
        // If no parameters are provided, we still need to remove the first token
        tokens.erase(tokens.begin());
    }
}

bool BaseProxy::is_aborting() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->abort_flag;
}

bool BaseProxy::cycle_start_allowed() {
    lock_guard<mutex> semaphore(this->api_mutex);
    switch (this->orchestration_schema) {
        case NONE:
            return true;
        case SYNC_ON_CYCLE_START:
            if (this->cycle_start_allowed_flag) {
                this->cycle_start_allowed_flag = false;
                return true;
            } else {
                return false;
            }
        default:
            RAISE_ERROR("Invalid orchestration schema: " + std::to_string(this->orchestration_schema));
            return false;
    }
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
        } else if (command == ALLOW_CYCLE_START) {
            allow_cycle_start(args);
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

void BaseProxy::allow_cycle_start(const vector<string>& args) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->cycle_start_allowed_flag = true;
}

// ---------------------------------------------------------------------------------------------
// Protected methods

void BaseProxy::set_orchestration_schema(ORCHESTRATION_SCHEMA_TYPE value) {
    lock_guard<mutex> semaphore(this->api_mutex);
    if ((value < NONE) || (value > SYNC_ON_CYCLE_START)) {
        RAISE_ERROR("Invalid orchestration tag: " + std::to_string(value));
    } else {
        this->orchestration_schema = value;
    }
}
