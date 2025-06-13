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

// -------------------------------------------------------------------------------------------------
// Server-side API

bool BaseProxy::is_aborting() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->abort_flag;
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
            return true;
        } else if (command == ABORT) {
            abort(args);
            return true;
        } else {
            Utils::error("Invalid proxy command: <" + command + ">");
        }
    }
    return false;
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
