#include "InferenceProxy.h"
#include "ServiceBus.h"


using namespace inference_agent;
InferenceProxy::InferenceProxy() : BaseQueryProxy() {
}

InferenceProxy::InferenceProxy(const vector<string>& tokens) : BaseQueryProxy() {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->command = ServiceBus::INFERENCE;
    this->args.insert(this->args.end(), tokens.begin(), tokens.end());
}

InferenceProxy::~InferenceProxy() {
}

void InferenceProxy::pack_command_line_args() {
    tokenize(this->args);
}

void InferenceProxy::set_default_parameters() {
}

void InferenceProxy::set_parameter(const string& key, const PropertyValue& value) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->parameters[key] = value;
}