#include "CommandRouterHttpAPI.h"

#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"
#include "Utils.h"

using namespace command_router;
using namespace commons;
using namespace processor;
using nlohmann::json;

CommandRouterHttpAPI::CommandRouterHttpAPI(const string& host,
                                           int port,
                                           shared_ptr<ThreadPool> thread_pool)
    : Processor("command_router_http_api"), host(host), port(port), thread_pool(thread_pool) {}

CommandRouterHttpAPI::~CommandRouterHttpAPI() { this->stop(); }

void CommandRouterHttpAPI::initialize(shared_ptr<CommandRouterHttpAPI> instance,
                                      vector<shared_ptr<Processor>> additional_subprocessors) {
    for (const auto& subprocessor : additional_subprocessors) {
        Processor::bind_subprocessor(instance, subprocessor);
    }
    instance->setup();
    instance->start();
}

bool CommandRouterHttpAPI::thread_one_step() {
    setup_routes();
    LOG_INFO("bus_command_router HTTP server listening on " << host << ":" << port);
    this->server.listen(host, port);
    return false;
}

void CommandRouterHttpAPI::stop() {
    LOG_INFO("bus_command_router HTTP server stop requested.");
    this->server.stop();
    Processor::stop();
}

// =============================================================================
// Route setup
// =============================================================================

void CommandRouterHttpAPI::setup_routes() {
    this->server.Get("/ping", [](const httplib::Request& req, httplib::Response& res) {
        LOG_INFO("bus_command_router HTTP server received health check request");
        res.status = 200;
        res.set_content("PONG!", "text/plain");
    });
}