#include "InferenceProxy.h"

#include "ServiceBus.h"

using namespace inference_agent;

string InferenceProxy::INFERENCE_REQUEST_TIMEOUT = "INFERENCE_REQUEST_TIMEOUT";
string InferenceProxy::UPDATE_ATTENTION_BROKER_FLAG = "UPDATE_ATTENTION_BROKER_FLAG";
string InferenceProxy::REPEAT_REQUEST_NUMBER = "REPEAT_REQUEST_NUMBER";
string InferenceProxy::MAX_QUERY_ANSWERS_TO_PROCESS = "MAX_QUERY_ANSWERS_TO_PROCESS";
string InferenceProxy::RUN_FULL_EVALUATION_QUERY = "RUN_FULL_EVALUATION_QUERY";

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
    this->parameters[UPDATE_ATTENTION_BROKER_FLAG] = (bool) false;
    this->parameters[REPEAT_REQUEST_NUMBER] = (unsigned int) 5;
    this->parameters[MAX_QUERY_ANSWERS_TO_PROCESS] = (unsigned int) 100;
    this->parameters[RUN_FULL_EVALUATION_QUERY] = (bool) false;
}

void InferenceProxy::set_parameter(const string& key, const PropertyValue& value) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->parameters[key] = value;
}
