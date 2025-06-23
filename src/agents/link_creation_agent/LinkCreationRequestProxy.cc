#include "LinkCreationRequestProxy.h"

#include "ServiceBus.h"
#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace link_creation_agent;

string LinkCreationRequestProxy::ABORT = "abort";
string LinkCreationRequestProxy::LINK_CREATION_FAILED = "link_creation_failed";

LinkCreationRequestProxy::LinkCreationRequestProxy() {
    lock_guard<mutex> semaphore(this->api_mutex);
    init();
}

LinkCreationRequestProxy::LinkCreationRequestProxy(const vector<string>& tokens) : BusCommandProxy() {
    lock_guard<mutex> semaphore(this->api_mutex);
    init();
    this->command = ServiceBus::LINK_CREATE_REQUEST;
    this->args.insert(this->args.end(), tokens.begin(), tokens.end());
}

void LinkCreationRequestProxy::init() { this->abort_flag = false; }

LinkCreationRequestProxy::~LinkCreationRequestProxy() {}

bool LinkCreationRequestProxy::finished() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->abort_flag;
}

void LinkCreationRequestProxy::abort() {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->abort_flag = true;
    this->to_remote_peer(ABORT, {});
}

void LinkCreationRequestProxy::raise_error(const string& error_message, unsigned int error_code) {
    string error = "Exception thrown in command processor.";
    if (error_code > 0) {
        error += " Error code: " + std::to_string(error_code);
    }
    error += "\n";
    error += error_message;
    this->error_code = error_code;
    this->error_message = error;
    // LOG_ERROR(error);
    throw runtime_error(error);
}
