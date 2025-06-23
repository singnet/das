#include "LinkCreationRequestProcessor.h"

#include "LinkCreationRequestProxy.h"
#include "Logger.h"

using namespace link_creation_agent;
LinkCreationRequestProcessor::LinkCreationRequestProcessor()
    : BusCommandProcessor({ServiceBus::LINK_CREATE_REQUEST}) {
    this->link_creation_agent = new LinkCreationAgent();
}

LinkCreationRequestProcessor::~LinkCreationRequestProcessor() { delete this->link_creation_agent; }

shared_ptr<BusCommandProxy> LinkCreationRequestProcessor::factory_empty_proxy() {
    shared_ptr<LinkCreationRequestProxy> proxy(new LinkCreationRequestProxy());
    return proxy;
}

void LinkCreationRequestProcessor::run_command(shared_ptr<BusCommandProxy> proxy) {
    auto request_proxy = dynamic_pointer_cast<LinkCreationRequestProxy>(proxy);
    if (request_proxy->get_command() == LinkCreationRequestProxy::ABORT) {
        abort_link_create_request(request_proxy);
    } else if (request_proxy->get_command() == ServiceBus::LINK_CREATE_REQUEST) {
        process_link_create_request(request_proxy);
    } else {
        proxy->raise_error_on_peer("Invalid command: " + request_proxy->get_command());
    }
}

void LinkCreationRequestProcessor::process_link_create_request(shared_ptr<BusCommandProxy> proxy) {
    auto request_proxy = dynamic_pointer_cast<LinkCreationRequestProxy>(proxy);
    if (request_proxy == nullptr) {
        proxy->raise_error_on_peer("Invalid proxy type for LINK_CREATE_REQUEST");
    }
    try {
        auto request = request_proxy->get_args();
        request.push_back(proxy->peer_id() +
                          to_string(proxy->get_serial()));  // Add request ID to the end of the request
        this->link_creation_agent->process_request(request_proxy->get_args());
    } catch (const std::exception& e) {
        LOG_ERROR("Error processing link creation request: " << e.what());
        request_proxy->raise_error_on_peer("Error processing link creation request: " +
                                           string(e.what()));
        return;
    }
}

void LinkCreationRequestProcessor::abort_link_create_request(shared_ptr<BusCommandProxy> proxy) {
    auto request_proxy = dynamic_pointer_cast<LinkCreationRequestProxy>(proxy);
    if (request_proxy == nullptr) {
        proxy->raise_error_on_peer("Invalid proxy type for ABORT command");
    }
    LOG_DEBUG("Aborting link creation request: " << request_proxy->get_args().back());
    this->link_creation_agent->abort_request(proxy->peer_id() + to_string(proxy->get_serial()));
}
