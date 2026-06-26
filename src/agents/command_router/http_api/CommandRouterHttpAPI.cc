#include "CommandRouterHttpAPI.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include "BusCommandRouterProcessor.h"
#include "BusCommandRouterProxy.h"
#include "BusCommandRouterProxyStreamPoller.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"
#include "Utils.h"
#include "expression_hasher.h"

using namespace command_router;
using namespace commons;
using namespace processor;
using namespace agents;

using json = nlohmann::json;

const unordered_set<string> CommandRouterHttpAPI::VALID_COMMAND_TYPES = {
    "query", "evolution", "get", "set"};

// -------------------------------------------------------------------------------------------------
// Constructors, destructors

CommandRouterHttpAPI::CommandRouterHttpAPI(const string& host,
                                           int port,
                                           shared_ptr<processor::ThreadPool> thread_pool,
                                           HttpAPISettings settings,
                                           shared_ptr<BusCommandRouterProcessor> router_processor,
                                           const string& bus_host)
    : Processor("command_router_http_api"),
      host(host),
      port(port),
      thread_pool(thread_pool),
      router_processor(std::move(router_processor)),
      bus_host(bus_host),
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
    {
        lock_guard<mutex> semaphore(this->executions_mtx);
        this->execution_slots_cv.notify_all();
    }
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

            if (!body.is_object() || !body.contains("command_type") ||
                !body["command_type"].is_string() || !body.contains("command_text") ||
                !body["command_text"].is_string()) {
                this->set_json_response(
                    response, 400, {{"error", "Missing fields: command_type, command_text"}});
                return;
            }

            string command_type = body["command_type"].get<string>();
            string command_text = body["command_text"].get<string>();

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

            switch (this->try_admit_execution(exec)) {
                case AdmitResult::ConcurrentLimit:
                    this->set_json_response(
                        response, 429, {{"error", "Maximum concurrent executions reached"}});
                    return;
                case AdmitResult::QueueFull:
                    this->set_json_response(response, 503, {{"error", "Execution queue is full"}});
                    return;
                case AdmitResult::Admitted:
                    break;
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
                    lock_guard<mutex> semaphore(this->executions_mtx);
                    this->executions.erase(exec->execution_id);
                    this->pending_executions--;
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
                        return next_index < exec->events.size() ||
                               CommandExecution::is_terminal(exec->status);
                    });

                    if (!has_new_event) continue;

                    if (next_index >= exec->events.size()) {
                        stream_finished = CommandExecution::is_terminal(exec->status);
                        continue;
                    }

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
    lock_guard<mutex> semaphore(this->executions_mtx);
    auto it = this->executions.find(execution_id);
    return (it != this->executions.end()) ? it->second : nullptr;
}

void CommandRouterHttpAPI::run_execution_inner(const shared_ptr<CommandExecution>& exec) {
    if (this->router_processor == nullptr) {
        lock_guard<mutex> lock(exec->mtx);
        exec->mark_error("Command router processor is not configured for HTTP execution");
        return;
    }

    const auto started_at = Utils::get_current_time_millis();
    int seq = 0;
    int received_count = 0;

    auto should_abort = [&]() {
        lock_guard<mutex> lock(exec->mtx);
        return exec->cancel_requested || this->shutting_down.load();
    };
    auto on_chunk = [&](const vector<string>& chunk) {
        lock_guard<mutex> lock(exec->mtx);
        received_count += static_cast<int>(chunk.size());
        exec->publish_chunk(++seq, chunk);
    };
    auto on_error = [&](const string& message) {
        lock_guard<mutex> lock(exec->mtx);
        exec->mark_error(message);
    };
    auto on_aborted = [&]() {
        lock_guard<mutex> lock(exec->mtx);
        exec->mark_aborted();
        LOG_INFO("CommandRouter HTTP API execution aborted id=" << exec->execution_id);
    };
    auto on_complete = [&](long long duration_ms, int total_items) {
        lock_guard<mutex> lock(exec->mtx);
        exec->mark_completed(duration_ms, total_items);
        LOG_INFO("CommandRouter HTTP API execution completed id="
                 << exec->execution_id << " duration_ms=" << duration_ms << " items=" << total_items);
    };

    try {
        string router_arg = exec->command_text;
        Utils::replace_all(router_arg, "%", "$");
        auto router_proxy = make_shared<BusCommandRouterProxy>(exec->command_type, router_arg);

        string http_requestor_id = this->bus_host + ":http-" + exec->execution_id;
        this->router_processor->dispatch_http_command(router_proxy, http_requestor_id);

        if (!BusCommandRouterProxyStreamPoller::poll_stream(router_proxy,
                                                            exec->command_type,
                                                            this->settings.stream_items_per_chunk,
                                                            should_abort,
                                                            on_chunk,
                                                            on_error,
                                                            on_aborted)) {
            return;
        }

        on_complete(Utils::get_current_time_millis() - started_at, received_count);
    } catch (const exception& e) {
        on_error(e.what());
    }
}

void CommandRouterHttpAPI::run_execution(const shared_ptr<CommandExecution>& exec) {
    {
        unique_lock<mutex> semaphore(this->executions_mtx);
        this->execution_slots_cv.wait(semaphore, [&] {
            return this->shutting_down.load() ||
                   this->running_executions < this->settings.max_concurrent_executions;
        });

        if (this->shutting_down.load()) {
            lock_guard<mutex> exec_semaphore(exec->mtx);
            if (!CommandExecution::is_terminal(exec->status)) {
                exec->mark_error("Server is shutting down");
            }
            this->pending_executions--;
            return;
        }

        {
            lock_guard<mutex> exec_semaphore(exec->mtx);
            exec->mark_running();
        }
        this->pending_executions--;
        this->running_executions++;
    }

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

    lock_guard<mutex> semaphore(this->executions_mtx);
    this->running_executions--;
    this->execution_slots_cv.notify_one();
}

void CommandRouterHttpAPI::cleanup_finished_executions() {
    if (this->settings.execution_retention_ms <= 0) {
        return;
    }

    const auto now = Utils::get_current_time_millis();
    lock_guard<mutex> semaphore(this->executions_mtx);
    for (auto it = this->executions.begin(); it != this->executions.end();) {
        auto exec = it->second;
        bool should_erase = false;
        {
            lock_guard<mutex> exec_semaphore(exec->mtx);
            const bool retention_expired =
                exec->finished_at_ms > 0 &&
                (now - exec->finished_at_ms) > this->settings.execution_retention_ms;
            should_erase = CommandExecution::is_terminal(exec->status) && retention_expired;
        }
        if (should_erase) {
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

CommandRouterHttpAPI::AdmitResult CommandRouterHttpAPI::try_admit_execution(
    const shared_ptr<CommandExecution>& exec) {
    lock_guard<mutex> semaphore(this->executions_mtx);
    if (this->running_executions >= this->settings.max_concurrent_executions) {
        return AdmitResult::ConcurrentLimit;
    }
    if (this->settings.max_queued_executions > 0 &&
        this->pending_executions >= this->settings.max_queued_executions) {
        return AdmitResult::QueueFull;
    }
    this->executions[exec->execution_id] = exec;
    this->pending_executions++;
    return AdmitResult::Admitted;
}

void CommandRouterHttpAPI::set_json_response(httplib::Response& response, int status, const json& body) {
    response.status = status;
    string content = body.dump();
    response.set_content(content, "application/json");
}
