#include "LinkCreationRequestProxy.h"
#include "ServiceBus.h"

using namespace link_creation_agent;

const string LinkCreationRequestProxy::Parameters::QUERY_INTERVAL =
    "link_creation_agent.requests_interval";
const string LinkCreationRequestProxy::Parameters::QUERY_TIMEOUT = "link_creation_agent.query_timeout";

const string LinkCreationRequestProxy::Parameters::ABORT_FLAG = "link_creation_agent.abort_flag";

LinkCreationRequestProxy::LinkCreationRequestProxy() : BaseProxy() {}

LinkCreationRequestProxy::LinkCreationRequestProxy(const vector<string>& tokens) : BaseProxy() {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->command = ServiceBus::LINK_CREATION;
    this->args.insert(this->args.end(), tokens.begin(), tokens.end());
}

LinkCreationRequestProxy::~LinkCreationRequestProxy() {}

void LinkCreationRequestProxy::pack_command_line_args() { tokenize(this->args); }

void LinkCreationRequestProxy::set_default_parameters() {
    this->parameters[LinkCreationRequestProxy::Parameters::QUERY_INTERVAL] = (unsigned int) 800;
    this->parameters[LinkCreationRequestProxy::Parameters::QUERY_TIMEOUT] = (unsigned int) 600;
}

void LinkCreationRequestProxy::set_parameter(const string& key, const PropertyValue& value) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->parameters[key] = value;
}