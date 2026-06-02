#include "LinkCreationRequestProxy.h"

#include "BaseQueryProxy.h"
#include "ServiceBus.h"
#include "SystemParametersSingleton.h"

using namespace link_creation_agent;

string LinkCreationRequestProxy::MAX_ANSWERS = "max_answers";
string LinkCreationRequestProxy::REPEAT_COUNT = "repeat_count";
string LinkCreationRequestProxy::CONTEXT = "context";
string LinkCreationRequestProxy::ATTENTION_UPDATE = "attention_update";
string LinkCreationRequestProxy::ATTENTION_CORRELATION = "attention_correlation";
string LinkCreationRequestProxy::POSITIVE_IMPORTANCE_FLAG = "positive_importance_flag";
string LinkCreationRequestProxy::QUERY_INTERVAL = "query_interval";
string LinkCreationRequestProxy::QUERY_TIMEOUT = "query_timeout";
string LinkCreationRequestProxy::USE_METTA_AS_QUERY_TOKENS = "use_metta_as_query_tokens";

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
    this->parameters = SystemParametersSingleton::get_instance()->get_link_creation_agent_params();
}

void LinkCreationRequestProxy::set_parameter(const string& key, const PropertyValue& value) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->parameters[key] = value;
}
