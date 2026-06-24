#include "CommandRouterHttpAPI.h"

#include <cctype>
#include <sstream>

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"
#include "Utils.h"
#include "expression_hasher.h"

using namespace command_router;
using namespace commons;
using namespace processor;

using json = nlohmann::json;

const unordered_set<string> CommandRouterHttpAPI::VALID_COMMAND_TYPES = {
    "query", "evolution", "get", "set"};

namespace {

HttpAPISettings load_http_api_settings(const JsonConfig& command_router_config) {
    HttpAPISettings settings;
    settings.max_concurrent_executions =
        command_router_config.at_path("http_api.max_concurrent_executions")
            .get_or<size_t>(settings.max_concurrent_executions);
    settings.max_queued_executions = command_router_config.at_path("http_api.max_queued_executions")
                                         .get_or<size_t>(settings.max_queued_executions);
    settings.max_events_per_execution =
        command_router_config.at_path("http_api.max_events_per_execution")
            .get_or<size_t>(settings.max_events_per_execution);
    settings.execution_retention_ms = command_router_config.at_path("http_api.execution_retention_ms")
                                          .get_or<long long>(settings.execution_retention_ms);
    return settings;
}

}  // namespace

// -------------------------------------------------------------------------------------------------
// Constructors, destructors

CommandRouterHttpAPI::CommandRouterHttpAPI(const string& host,
                                           int port,
                                           shared_ptr<processor::ThreadPool> thread_pool,
                                           HttpAPISettings settings)
    : Processor("command_router_http_api"),
      host(host),
      port(port),
      thread_pool(thread_pool),
      settings(std::move(settings)) {
    if (this->thread_pool == nullptr) {
        RAISE_ERROR("CommandRouterHttpAPI requires a non-null thread pool");
    }
}

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
    LOG_INFO("CommandRouter HTTP API"
             << " listening on " << instance->host << ":" << instance->port);
}

HttpAPISettings CommandRouterHttpAPI::settings_from_config(const JsonConfig& command_router_config) {
    return load_http_api_settings(command_router_config);
}

void CommandRouterHttpAPI::setup() {
    this->setup_routes();
    Processor::setup();
}

bool CommandRouterHttpAPI::thread_one_step() {
    if (this->shutting_down.load()) return false;

    if (!this->server.listen_after_bind() && !this->shutting_down.load()) {
        LOG_ERROR("CommandRouter HTTP API failed to bind " << this->host << ":" << this->port);
    }

    return false;
}

void CommandRouterHttpAPI::setup() {
    this->setup_routes();
    if (!this->server.bind_to_port(this->host.c_str(), this->port)) {
        RAISE_ERROR("CommandRouter HTTP API failed to bind on " + this->host + ":" +
                    std::to_string(this->port));
    }
    Processor::setup();
}

void CommandRouterHttpAPI::stop() {
    this->shutting_down = true;
    LOG_INFO("CommandRouter HTTP API stopping on " << this->host << ":" << this->port);
    this->server.stop();
    this->cleanup_finished_executions();
    if (this->is_running()) {
        Processor::stop();
    }
}

// -------------------------------------------------------------------------
// Private methods

void CommandRouterHttpAPI::setup_routes() {
    // GET /ping for health checks
    this->server.Get("/ping", [](const httplib::Request& request, httplib::Response& response) {
        LOG_DEBUG("CommandRouter HTTP API GET /ping");
        response.status = 200;
        response.set_content("PONG!", "text/plain");
    });

    // POST /command-router/executions
    this->server.Post(
        "/command-router/executions",
        [this](const httplib::Request& request, httplib::Response& response) {
            if (this->shutting_down.load()) {
                this->set_json_response(response, 503, {{"error", "Server is shutting down"}});
                return;
            }

            this->cleanup_finished_executions();

            json body;

            try {
                body = json::parse(request.body);
            } catch (const exception& e) {
                this->set_json_response(response, 400, {{"error", string("Invalid JSON: ") + e.what()}});
                return;
            }

            string command_type = body.value("command_type", "");
            string command_text = body.value("command_text", "");

            if (command_type.empty() || command_text.empty()) {
                this->set_json_response(
                    response, 400, {{"error", "Missing fields: command_type, command_text"}});
                return;
            }

            if (!this->is_valid_command_type(command_type)) {
                this->set_json_response(
                    response,
                    400,
                    {{"error", "Invalid command_type. Allowed values: query, evolution, get, set"}});
                return;
            }

            auto exec = make_shared<CommandExecution>(this->generate_execution_id(),
                                                      command_type,
                                                      command_text,
                                                      this->settings.max_events_per_execution);

            size_t active_count = 0;
            if (!this->try_admit_execution(exec, active_count)) {
                if (active_count >= this->settings.max_concurrent_executions) {
                    this->set_json_response(
                        response, 429, {{"error", "Maximum concurrent executions reached"}});
                } else {
                    this->set_json_response(response, 503, {{"error", "Execution queue is full"}});
                }
                return;
            }

            string status_for_response;
            {
                lock_guard<mutex> exec_semaphore(exec->mtx);
                status_for_response = CommandExecution::status_to_string(exec->status);
            }

            LOG_INFO("CommandRouter HTTP API execution scheduled id=" << exec->execution_id
                                                                      << " type=" << command_type);

            try {
                this->thread_pool->enqueue([this, exec]() { this->run_execution(exec); });
            } catch (const exception& e) {
                {
                    lock_guard<mutex> semaphore(this->executions_mutex);
                    this->executions.erase(exec->execution_id);
                }
                this->set_json_response(
                    response, 503, {{"error", string("Failed to schedule execution: ") + e.what()}});
                return;
            }

            this->set_json_response(response,
                                    202,
                                    {
                                        {"execution_id", exec->execution_id},
                                        {"status", status_for_response},
                                    });
        });

    // WebSocket  WS /command-router/ws/:id
    this->server.WebSocket(
        R"(/command-router/ws/([^/]+))",
        [this](const httplib::Request& request, httplib::ws::WebSocket& ws) {
            const string execution_id = request.matches[1];

            auto exec = this->find_execution(execution_id);

            if (!exec) {
                ws.close(httplib::ws::CloseStatus::PolicyViolation,
                         "Execution not found: " + execution_id);
                LOG_INFO("CommandRouter HTTP API WebSocket rejected id=" << execution_id
                                                                         << " (not found)");
                return;
            }

            LOG_INFO("CommandRouter HTTP API WebSocket open id=" << execution_id);

            size_t next_index = 0;
            bool stream_finished = false;

            while (!stream_finished && ws.is_open()) {
                string payload;

                {
                    unique_lock<mutex> exec_semaphore(exec->mtx);

                    bool has_new_event = exec->cv.wait_for(exec_semaphore, chrono::seconds(1), [&] {
                        return next_index < exec->events.size();
                    });

                    if (!has_new_event) continue;

                    payload = exec->events[next_index++];
                    stream_finished =
                        CommandExecution::is_terminal(exec->status) && next_index >= exec->events.size();
                }

                bool send_ok = ws.send(payload);

                if (!send_ok) {
                    LOG_INFO("CommandRouter HTTP API WebSocket send failed id=" << execution_id);
                    return;
                }
            }

            ws.close(httplib::ws::CloseStatus::Normal, "stream complete");

            LOG_INFO("CommandRouter HTTP API WebSocket closed id=" << execution_id);
        });

    // GET /command-router/executions/:id
    this->server.Get(R"(/command-router/executions/([^/]+))",
                     [this](const httplib::Request& request, httplib::Response& response) {
                         const string execution_id = request.matches[1];

                         auto exec = this->find_execution(execution_id);

                         if (!exec) {
                             this->set_json_response(
                                 response, 404, {{"error", "Execution not found: " + execution_id}});
                             return;
                         }

                         lock_guard<mutex> exec_semaphore(exec->mtx);

                         LOG_DEBUG("CommandRouter HTTP API GET /command-router/executions/"
                                   << execution_id
                                   << " status=" << CommandExecution::status_to_string(exec->status));

                         json success_body = {
                             {"execution_id", exec->execution_id},
                             {"status", CommandExecution::status_to_string(exec->status)},
                             {"received_count", exec->received_count},
                             {"total_items", exec->total_items},
                             {"duration_ms", exec->duration_ms},
                         };

                         if (!exec->error_message.empty()) {
                             success_body["error_message"] = exec->error_message;
                         }

                         this->set_json_response(response, 200, success_body);
                     });

    // POST /command-router/executions/:id/cancel
    this->server.Post(
        R"(/command-router/executions/([^/]+)/cancel)",
        [this](const httplib::Request& request, httplib::Response& response) {
            const string execution_id = request.matches[1];

            auto exec = this->find_execution(execution_id);

            if (!exec) {
                this->set_json_response(
                    response, 404, {{"error", "Execution not found: " + execution_id}});
                return;
            }

            ExecutionStatus current_status;
            {
                lock_guard<mutex> exec_semaphore(exec->mtx);
                current_status = exec->status;

                if (CommandExecution::is_terminal(current_status)) {
                    this->set_json_response(
                        response,
                        409,
                        {{"execution_id", execution_id},
                         {"status", CommandExecution::status_to_string(current_status)},
                         {"error", "Execution is already in a terminal state"}});
                    return;
                }

                exec->cancel_requested = true;
            }

            exec->cv.notify_all();

            LOG_INFO("CommandRouter HTTP API execution cancel requested id=" << execution_id);

            this->set_json_response(
                response, 200, {{"execution_id", execution_id}, {"status", "cancel_requested"}});
        });
}

shared_ptr<CommandExecution> CommandRouterHttpAPI::find_execution(const string& execution_id) {
    lock_guard<mutex> semaphore(this->executions_mutex);
    auto it = this->executions.find(execution_id);
    return (it != this->executions.end()) ? it->second : nullptr;
}

void CommandRouterHttpAPI::run_execution_inner(const shared_ptr<CommandExecution>& exec) {
    {
        lock_guard<mutex> lock(exec->mtx);
        exec->mark_running();
    }

    // POC code to simulate event streaming.

    const int TOTAL = 1000;
    const int CHUNK_SIZE = 10;
    auto started_at = Utils::get_current_time_millis();
    int seq = 0;

    for (int i = 0; i < TOTAL; i += CHUNK_SIZE) {
        {
            lock_guard<mutex> lock(exec->mtx);
            if (exec->cancel_requested) {
                exec->mark_aborted();
                LOG_INFO("CommandRouter HTTP API execution aborted id=" << exec->execution_id);
                // this->cleanup_finished_executions(); WIP
                return;
            }
        }

        Utils::sleep(200);

        vector<string> chunk_data;
        for (int k = i; k < min(i + CHUNK_SIZE, TOTAL); ++k) {
            chunk_data.push_back("result-" + std::to_string(k));
        }

        lock_guard<mutex> lock(exec->mtx);
        exec->publish_chunk(++seq, chunk_data);
    }

    const auto duration_ms = Utils::get_current_time_millis() - started_at;
    lock_guard<mutex> lock(exec->mtx);
    exec->mark_completed(duration_ms, exec->received_count);

    LOG_INFO("CommandRouter HTTP API execution completed id=" << exec->execution_id
                                                              << " duration_ms=" << duration_ms
                                                              << " items=" << exec->received_count);
}

void CommandRouterHttpAPI::run_execution(const shared_ptr<CommandExecution>& exec) {
    try {
        this->run_execution_inner(exec);
    } catch (const exception& e) {
        LOG_ERROR("CommandRouter HTTP API execution failed id=" << exec->execution_id << ": "
                                                                << e.what());
        lock_guard<mutex> lock(exec->mtx);
        if (!CommandExecution::is_terminal(exec->status)) {
            exec->mark_error(string("Execution failed: ") + e.what());
        }
    } catch (...) {
        LOG_ERROR("CommandRouter HTTP API execution failed id=" << exec->execution_id
                                                                << ": unknown error");
        lock_guard<mutex> lock(exec->mtx);
        if (!CommandExecution::is_terminal(exec->status)) {
            exec->mark_error("Execution failed: unknown error");
        }
    }
}

void CommandRouterHttpAPI::cleanup_finished_executions() {
    if (this->settings.execution_retention_ms <= 0) {
        return;
    }

    const auto now = Utils::get_current_time_millis();
    lock_guard<mutex> semaphore(this->executions_mutex);
    for (auto it = this->executions.begin(); it != this->executions.end();) {
        lock_guard<mutex> exec_semaphore(it->second->mtx);
        const auto& exec = it->second;
        const bool retention_expired =
            exec->finished_at_ms > 0 &&
            now - exec->finished_at_ms >= this->settings.execution_retention_ms;
        if (CommandExecution::is_terminal(exec->status) && retention_expired) {
            it = this->executions.erase(it);
        } else {
            ++it;
        }
    }
}

string CommandRouterHttpAPI::generate_execution_id() {
    static atomic<uint64_t> counter{0};
    auto ms = Utils::get_current_time_millis();
    auto seed = std::to_string(ms) + "-" + std::to_string(counter.fetch_add(1));
    char* hash = compute_hash((char*) seed.c_str());
    string execution_id = string("exec-") + hash;
    delete[] hash;
    return execution_id;
}

bool CommandRouterHttpAPI::is_valid_command_type(const string& command_type) const {
    return this->VALID_COMMAND_TYPES.find(command_type) != this->VALID_COMMAND_TYPES.end();
}

size_t CommandRouterHttpAPI::count_active_executions_locked() const {
    size_t active_count = 0;
    for (const auto& entry : this->executions) {
        lock_guard<mutex> exec_semaphore(entry.second->mtx);
        if (!CommandExecution::is_terminal(entry.second->status)) {
            ++active_count;
        }
    }
    return active_count;
}

bool CommandRouterHttpAPI::try_admit_execution(const shared_ptr<CommandExecution>& exec,
                                               size_t& active_count) {
    lock_guard<mutex> semaphore(this->executions_mutex);
    active_count = this->count_active_executions_locked();
    if (active_count >= this->settings.max_concurrent_executions) {
        return false;
    }
    if (this->settings.max_queued_executions > 0 &&
        this->thread_pool->size() >= static_cast<int>(this->settings.max_queued_executions)) {
        return false;
    }
    this->executions[exec->execution_id] = exec;
    return true;
}

void CommandRouterHttpAPI::set_json_response(httplib::Response& response, int status, const json& body) {
    response.status = status;
    string content = body.dump();
    response.set_content(content, "application/json");
}
