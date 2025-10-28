#include "LinkCreationRequestProxy.h"

#include "BaseQueryProxy.h"
#include "ServiceBus.h"

using namespace link_creation_agent;

string LinkCreationRequestProxy::MAX_ANSWERS = "max_answers";
string LinkCreationRequestProxy::REPEAT_COUNT = "repeat_count";
string LinkCreationRequestProxy::CONTEXT = "context";
string LinkCreationRequestProxy::ATTENTION_UPDATE_FLAG = "attention_update_flag";
string LinkCreationRequestProxy::POSITIVE_IMPORTANCE_FLAG = "positive_importance_flag";
string LinkCreationRequestProxy::QUERY_INTERVAL = "query_interval";
string LinkCreationRequestProxy::QUERY_TIMEOUT = "query_timeout";

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
    this->parameters[LinkCreationRequestProxy::MAX_ANSWERS] = (unsigned int) 10;
    this->parameters[LinkCreationRequestProxy::REPEAT_COUNT] = (unsigned int) 1;
    this->parameters[LinkCreationRequestProxy::CONTEXT] = string("");
    this->parameters[LinkCreationRequestProxy::ATTENTION_UPDATE_FLAG] = false;
    this->parameters[LinkCreationRequestProxy::POSITIVE_IMPORTANCE_FLAG] = true;
    this->parameters[LinkCreationRequestProxy::QUERY_INTERVAL] = (unsigned int) 0;
    this->parameters[LinkCreationRequestProxy::QUERY_TIMEOUT] = (unsigned int) 0;
}

void LinkCreationRequestProxy::set_parameter(const string& key, const PropertyValue& value) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->parameters[key] = value;
}