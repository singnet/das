#include "InferenceProcessor.h"
#include "Logger.h"
#include "ServiceBus.h"

using namespace inference_agent;
using namespace service_bus;
InferenceProcessor::InferenceProcessor() : BusCommandProcessor({ServiceBus::INFERENCE}) {
    this->inference_agent = new InferenceAgent();
}

InferenceProcessor::~InferenceProcessor() {
    delete inference_agent;
}

shared_ptr<BusCommandProxy> InferenceProcessor::factory_empty_proxy() {
    return make_shared<InferenceProxy>();
}

void InferenceProcessor::run_command(shared_ptr<BusCommandProxy> proxy) {
    auto inference_proxy = dynamic_pointer_cast<InferenceProxy>(proxy);
    inference_proxy->untokenize(inference_proxy->args);
    if (inference_proxy->get_command() == ServiceBus::INFERENCE) {
        process_inference_request(inference_proxy);
    } else {
        proxy->raise_error_on_peer("Invalid command: " + inference_proxy->get_command());
    }
}

void InferenceProcessor::process_inference_request(shared_ptr<BusCommandProxy> proxy) {
    auto inference_proxy = dynamic_pointer_cast<InferenceProxy>(proxy);
    if (inference_proxy == nullptr) {
        proxy->raise_error_on_peer("Invalid proxy type for INFERENCE");
    }
    try {
        LOG_DEBUG("Processing inference request: " << Utils::join(inference_proxy->get_args(), ' '));
        inference_agent->process_inference_request(inference_proxy);
    } catch (const std::exception& e) {
        LOG_ERROR("Error processing inference request: " << e.what());
        inference_proxy->raise_error_on_peer("Error processing inference request: " + string(e.what()));
    }
}

