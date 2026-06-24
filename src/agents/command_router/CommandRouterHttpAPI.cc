#include "CommandRouterHttpAPI.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"
#include "Utils.h"

using namespace command_router;
using namespace commons;
using namespace processor;

// -------------------------------------------------------------------------------------------------
// Constructors, destructors

CommandRouterHttpAPI::CommandRouterHttpAPI(const string& host,
                                           int port,
                                           shared_ptr<processor::ThreadPool> thread_pool)
    : Processor("command_router_http_api"), host(host), port(port), thread_pool(thread_pool) {}

CommandRouterHttpAPI::~CommandRouterHttpAPI() { this->stop(); }

// -------------------------------------------------------------------------------------------------
// Public methods

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
    LOG_INFO("bus_command_router HTTP server stop requested on " << host << ":" << port);
    this->server.stop();
    if (this->is_running()) {
        Processor::stop();
    }
}

// -------------------------------------------------------------------------
// Private methods

// =============================================================================
// Route setup
// =============================================================================

void CommandRouterHttpAPI::setup_routes() {
    // GET /ping for health checks
    this->server.Get("/ping", [](const httplib::Request& request, httplib::Response& response) {
        LOG_DEBUG("bus_command_router HTTP server received health check request");
        response.status = 200;
        response.set_content("PONG!", "text/plain");
    });
}

shared_ptr<CommandRouterHttpAPI::Execution> CommandRouterHttpAPI::find_execution(
    const string& execution_id) {
    return shared_ptr<CommandRouterHttpAPI::Execution>();
}

void CommandRouterHttpAPI::run_execution(const shared_ptr<CommandRouterHttpAPI::Execution>& exec) {}

void CommandRouterHttpAPI::append_event_locked(const shared_ptr<CommandRouterHttpAPI::Execution>& exec,
                                               const JsonConfig& payload) {}

void CommandRouterHttpAPI::cleanup_finished_executions() {}

string CommandRouterHttpAPI::generate_execution_id() { return string(); }

JsonConfig CommandRouterHttpAPI::started_event(const string& execution_id) { return JsonConfig(); }

JsonConfig CommandRouterHttpAPI::chunk_event(const string& execution_id,
                                             int seq,
                                             const vector<string>& data,
                                             int received_count) {
    return JsonConfig();
}

JsonConfig CommandRouterHttpAPI::completed_event(const string& execution_id,
                                                 long long duration_ms,
                                                 int total_items) {
    return JsonConfig();
}

JsonConfig CommandRouterHttpAPI::error_event(const string& execution_id, const string& message) {
    return JsonConfig();
}

JsonConfig CommandRouterHttpAPI::aborted_event(const string& execution_id) { return JsonConfig(); }

void CommandRouterHttpAPI::set_json_response(httplib::Response& res,
                                             int status_code,
                                             const JsonConfig& payload) {}
