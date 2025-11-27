#include "LinkCreationRequestProcessor.h"

#include "LinkCreationRequestProxy.h"
#define LOG_LEVEL DEBUG_LEVEL
#include "Logger.h"

using namespace link_creation_agent;
LinkCreationRequestProcessor::LinkCreationRequestProcessor(int request_interval,
                                                           int thread_count,
                                                           int default_timeout,
                                                           string buffer_file_path,
                                                           string metta_file_path,
                                                           bool save_links_to_metta_file,
                                                           bool save_links_to_db)
    : BusCommandProcessor({ServiceBus::LINK_CREATION}) {
    this->link_creation_agent = new LinkCreationAgent(request_interval,
                                                      thread_count,
                                                      default_timeout,
                                                      buffer_file_path,
                                                      metta_file_path,
                                                      save_links_to_metta_file,
                                                      save_links_to_db);
}

LinkCreationRequestProcessor::LinkCreationRequestProcessor(const Properties& props)
    : BusCommandProcessor({ServiceBus::LINK_CREATION}) {
    int request_interval = Utils::string_to_int(props.get_or<string>("request_interval", "1"));
    int thread_count = Utils::string_to_int(props.get_or<string>("thread_count", "1"));
    int default_timeout = Utils::string_to_int(props.get_or<string>("default_timeout", "10"));
    string buffer_file_path = props.get_or<string>("buffer_file", "requests_buffer.bin");
    string metta_file_path = props.get_or<string>("metta_file_path", "/tmp/metta_links");
    bool save_links_to_metta_file = props.get_or<string>("save_links_to_metta_file", "false") == "true";
    bool save_links_to_db = props.get_or<string>("save_links_to_db", "true") == "true";

    this->link_creation_agent = new LinkCreationAgent(request_interval,
                                                      thread_count,
                                                      default_timeout,
                                                      buffer_file_path,
                                                      metta_file_path,
                                                      save_links_to_metta_file,
                                                      save_links_to_db);
}

LinkCreationRequestProcessor::~LinkCreationRequestProcessor() { delete this->link_creation_agent; }

shared_ptr<BusCommandProxy> LinkCreationRequestProcessor::factory_empty_proxy() {
    shared_ptr<LinkCreationRequestProxy> proxy(new LinkCreationRequestProxy());
    return proxy;
}

void LinkCreationRequestProcessor::run_command(shared_ptr<BusCommandProxy> proxy) {
    auto request_proxy = dynamic_pointer_cast<LinkCreationRequestProxy>(proxy);
    request_proxy->untokenize(request_proxy->args);
    if (request_proxy->get_command() == ServiceBus::LINK_CREATION) {
        process_link_create_request(request_proxy);
    } else {
        proxy->raise_error_on_peer("Invalid command: " + request_proxy->get_command());
    }
}

void LinkCreationRequestProcessor::process_link_create_request(shared_ptr<BusCommandProxy> proxy) {
    auto request_proxy = dynamic_pointer_cast<LinkCreationRequestProxy>(proxy);
    if (request_proxy == nullptr) {
        proxy->raise_error_on_peer("Invalid proxy type for LINK_CREATION");
    }
    try {
        LOG_DEBUG("Processing link creation request: " << Utils::join(request_proxy->get_args(), ' '));
        this->link_creation_agent->process_request(request_proxy);
    } catch (const std::exception& e) {
        LOG_ERROR("Error processing link creation request: " << e.what());
        request_proxy->raise_error_on_peer("Error processing link creation request: " +
                                           string(e.what()));
        return;
    }
}
