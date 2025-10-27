#include "LinkCreationRequestProxy.h"

#include "ServiceBus.h"

using namespace link_creation_agent;

const string LinkCreationRequestProxy::MAX_RESULTS = "max_results";
const string LinkCreationRequestProxy::REPEAT_COUNT = "repeat_count";
const string LinkCreationRequestProxy::INFINITE_REQUEST = "infinite_request";
const string LinkCreationRequestProxy::CONTEXT = "context";
const string LinkCreationRequestProxy::UPDATE_ATTENTION_BROKER = "update_attention_broker";
const string LinkCreationRequestProxy::IMPORTANCE_FLAG = "importance_flag";
const string LinkCreationRequestProxy::QUERY_INTERVAL = "query_interval";
const string LinkCreationRequestProxy::QUERY_TIMEOUT = "query_timeout";

LinkCreationRequestProxy::LinkCreationRequestProxy() : BaseProxy() {}

LinkCreationRequestProxy::LinkCreationRequestProxy(const vector<string>& tokens) : BaseProxy() {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->command = ServiceBus::LINK_CREATION;
    set_default_parameters();
    this->args.insert(this->args.end(), tokens.begin(), tokens.end());
}

LinkCreationRequestProxy::~LinkCreationRequestProxy() {}

void LinkCreationRequestProxy::pack_command_line_args() { tokenize(this->args); }

void LinkCreationRequestProxy::set_default_parameters() {
    this->parameters[LinkCreationRequestProxy::MAX_RESULTS] = (unsigned int) 10;
    this->parameters[LinkCreationRequestProxy::REPEAT_COUNT] = (unsigned int) 1;
    this->parameters[LinkCreationRequestProxy::INFINITE_REQUEST] = false;
    this->parameters[LinkCreationRequestProxy::CONTEXT] = string("");
    this->parameters[LinkCreationRequestProxy::UPDATE_ATTENTION_BROKER] = false;
    this->parameters[LinkCreationRequestProxy::IMPORTANCE_FLAG] = true;
    this->parameters[LinkCreationRequestProxy::QUERY_INTERVAL] = (unsigned int) 0;
    this->parameters[LinkCreationRequestProxy::QUERY_TIMEOUT] = (unsigned int) 0;
}

void LinkCreationRequestProxy::set_parameter(const string& key, const PropertyValue& value) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->parameters[key] = value;
}