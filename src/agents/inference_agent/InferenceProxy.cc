#include "InferenceProxy.h"

#include "ServiceBus.h"

using namespace inference_agent;

string InferenceProxy::INFERENCE_REQUEST_TIMEOUT = "inference_request_timeout";
string InferenceProxy::UPDATE_ATTENTION_BROKER = "update_attention_broker";
string InferenceProxy::REPEAT_COUNT = "repeat_count";
string InferenceProxy::MAX_RESULTS = "max_results";
string InferenceProxy::RUN_FULL_EVALUATION_QUERY = "run_full_evaluation_query";

InferenceProxy::InferenceProxy() : BaseQueryProxy() {}

InferenceProxy::InferenceProxy(const vector<string>& tokens) : BaseQueryProxy() {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->command = ServiceBus::INFERENCE;
    set_default_parameters();
    this->args.insert(this->args.end(), tokens.begin(), tokens.end());
}

InferenceProxy::~InferenceProxy() {}

void InferenceProxy::pack_command_line_args() { tokenize(this->args); }

void InferenceProxy::set_default_parameters() {
    this->parameters[INFERENCE_REQUEST_TIMEOUT] =
        (unsigned int) 24 * 60 * 60;  // Default timeout is 24 hours
    this->parameters[UPDATE_ATTENTION_BROKER] = (bool) false;
    this->parameters[REPEAT_COUNT] = (unsigned int) 5;
    this->parameters[MAX_RESULTS] = (unsigned int) 150;
    this->parameters[RUN_FULL_EVALUATION_QUERY] = (bool) false;
}

void InferenceProxy::set_parameter(const string& key, const PropertyValue& value) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->parameters[key] = value;
}
