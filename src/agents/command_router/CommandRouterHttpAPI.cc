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

using json = nlohmann::json;

CommandRouterHttpAPI::CommandRouterHttpAPI(const string& host,
                                           int port,
                                           shared_ptr<ThreadPool> thread_pool)
    : Processor("command_router_http_api"), host(host), port(port), thread_pool(thread_pool) {}

CommandRouterHttpAPI::~CommandRouterHttpAPI() { this->stop(); }

void CommandRouterHttpAPI::initialize(shared_ptr<CommandRouterHttpAPI> instance,
                                      vector<shared_ptr<Processor>> additional_subprocessors) {
    // LOG_INFO("[[ 1 ]]");
    for (const auto& subprocessor : additional_subprocessors) {
        Processor::bind_subprocessor(instance, subprocessor);
    }
    // LOG_INFO("[[ 2 ]]");
    instance->setup();
    // LOG_INFO("[[ 3 ]]");
    instance->start();
    // LOG_INFO("[[ 4 ]]");
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

    // GET /ping for health checks
    this->server.Get("/ping", [](const httplib::Request& request, httplib::Response& response) {
        LOG_INFO("bus_command_router HTTP server received health check request");
        response.status = 200;
        response.set_content("PONG!", "text/plain");
    });

    // POST /command-router/executions
    // this->server.Post(
    //     "/command-router/executions",
    //     [this](const httplib::Request& request, httplib::Response& response) {
    //         JsonConfig body;
            
    //         try {
    //             body = JsonConfig(json::parse(request.body));
    //         } catch (const exception& e) {
    //             json error_body = {{"error", string("Invalid JSON: ") + e.what()}};
    //             set_json_response(response, 400, JsonConfig(error_body));
    //             return;
    //         }

    //         string command_type = body.at_path("command_type").get_or<string>("");
    //         string command_text = body.at_path("command_text").get_or<string>("");

    //         if (command_type.empty() || command_text.empty()) {
    //             json error_body = {{"error", "Missing fields: command_type, command_text"}};
    //             set_json_response(response, 400, JsonConfig(error_body));
    //             return;
    //         }

    //         auto exec = make_shared<Execution>();
    //         exec->execution_id = generate_execution_id();
    //         exec->command_type = command_type;
    //         exec->command_text = command_text;

    //         {
    //             lock_guard<mutex> semaphore(this->executions_mutex);
    //             this->executions[exec->execution_id] = exec;
    //         }

    //         thread_pool->enqueue([this, exec]() { this->run_execution(exec); });

    //         json success_body = {{"execution_id", exec->execution_id}, {"status", "pending"}};

    //         set_json_response(response, 202, JsonConfig(success_body));
    //     }
    // );

    // // GET /command-router/executions/:id
    // this->server.Get(
    //     R"(/command-router/executions/([^/]+))",
    //     [this](const httplib::Request& request, httplib::Response& response) {
    //         const string execution_id = request.matches[1];
    //         auto exec = find_execution(execution_id);

    //         if (!exec) {
    //             json error_body = {{"error", "Execution not found: " + execution_id}};
    //             set_json_response(response, 404, JsonConfig(error_body));
    //             return;
    //         }

    //         lock_guard<mutex> lk(exec->mtx);
            
    //         json success_body = {
    //             {"execution_id", exec->execution_id},
    //             {"status", exec->status},
    //             {"received_count", exec->received_count},
    //             {"total_items", exec->total_items},
    //             {"duration_ms", exec->duration_ms},
    //         };

    //         if (!exec->error_message.empty()) {
    //             success_body["error_message"] = exec->error_message;
    //         }

    //         set_json_response(response, 200, JsonConfig(success_body));
    //     }
    // );

    // // POST /command-router/executions/:id/cancel
    // this->server.Post(
    //     R"(/command-router/executions/([^/]+)/cancel)",
    //     [this](const httplib::Request& request, httplib::Response& response) {
    //         const string execution_id = request.matches[1];
    //         auto exec = find_execution(execution_id);
    //         if (!exec) {
    //             set_json_response(
    //                 response, 404, JsonConfig({{"error", "Execution not found: " + execution_id}}));
    //             return;
    //         }

    //         {
    //             lock_guard<mutex> lk(exec->mtx);
    //             exec->cancel_requested = true;
    //         }
    //         exec->cv.notify_all();

    //         set_json_response(
    //             response, 200, JsonConfig({{"execution_id", execution_id}, {"status", "cancel_requested"}}));
    //     }
    // );

    // WebSocket  WS /command-router/ws/:id    
    // this->server.WebSocket(
    //     R"(/command-router/ws/([^/]+))",
    //     [this](const httplib::Request& request, httplib::ws::WebSocket& ws) {
    //         const string execution_id = request.matches[1];
    //         auto exec = find_execution(execution_id);
            
    //         if (!exec) {
    //             ws.close(httplib::ws::CloseStatus::PolicyViolation, "Execution not found: " + execution_id);
    //             return;
    //         }

    //         LOG_INFO("CommandRouterHttpAPI: WebSocket connected for execution " << execution_id);

    //         size_t next_index = 0;
    //         bool terminal = false;

    //         while (!terminal && ws.is_open()) {
    //             string payload;
    //             {
    //                 // Wait up to 1 s for a new event to arrive.
    //                 unique_lock<mutex> lk(exec->mtx);
    //                 exec->cv.wait_for(lk, chrono::seconds(1), [&] {
    //                     return next_index < exec->events.size();
    //                 });

    //                 if (next_index >= exec->events.size()) {
    //                     continue;  // timeout, loop again
    //                 }
    //                 payload = exec->events[next_index++];
    //             }

    //             if (!ws.send(payload)) {
    //                 LOG_INFO("CommandRouterHttpAPI: WebSocket send failed for " << execution_id);
    //                 return;
    //             }

    //             // Detect end-of-stream events.
    //             try {
    //                 auto j = json::parse(payload);
    //                 string type = j.value("type", "");
    //                 if (type == "completed" || type == "error" || type == "aborted") {
    //                     terminal = true;
    //                 }
    //             } catch (...) {
    //             }
    //         }

    //         ws.close(httplib::ws::CloseStatus::Normal, "stream complete");
    //         LOG_INFO("CommandRouterHttpAPI: WebSocket closed for execution " << execution_id);
    //     }
    // );
}

// =============================================================================
// Execution logic
// =============================================================================

void CommandRouterHttpAPI::run_execution(const shared_ptr<Execution>& exec) {
    {
        lock_guard<mutex> lk(exec->mtx);
        exec->status = "running";
        append_event_locked(exec, started_event(exec->execution_id));
    }

    // TODO: Replace the simulation below with the real DAS command adapter.
    //       The adapter should:
    //         1. Translate command_type + command_text into a DAS command.
    //         2. Read output incrementally (via callback or stdout).
    //         3. Call append_event_locked() for each chunk.
    //         4. Check exec->cancel_requested to abort early.

    const int TOTAL = 100;
    const int CHUNK_SIZE = 10;
    auto started_at = chrono::steady_clock::now();
    int seq = 0;

    for (int i = 0; i < TOTAL; i += CHUNK_SIZE) {
        {
            lock_guard<mutex> lk(exec->mtx);
            if (exec->cancel_requested) {
                exec->status = "aborted";
                this->append_event_locked(exec, this->aborted_event(exec->execution_id));
                this->cleanup_finished_executions();
                return;
            }
        }

        Utils::sleep(200);

        vector<string> chunk_data;
        for (int k = i; k < min(i + CHUNK_SIZE, TOTAL); ++k) {
            chunk_data.push_back("result-" + std::to_string(k));
        }

        lock_guard<mutex> lk(exec->mtx);
        exec->received_count += static_cast<int>(chunk_data.size());
        this->append_event_locked(exec,
                            this->chunk_event(exec->execution_id, ++seq, chunk_data, exec->received_count));
    }

    lock_guard<mutex> lk(exec->mtx);
    exec->duration_ms = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - started_at).count();
    exec->total_items = exec->received_count;
    exec->status = "completed";
    this->append_event_locked(exec, this->completed_event(exec->execution_id, exec->duration_ms, exec->total_items));
}

// =============================================================================
// Event buffering
// =============================================================================

void CommandRouterHttpAPI::append_event_locked(const shared_ptr<Execution>& exec, const JsonConfig& payload) {
    exec->events.push_back(static_cast<const json&>(payload).dump());
    exec->cv.notify_all();
}

// =============================================================================
// Cleanup
// =============================================================================

void CommandRouterHttpAPI::cleanup_finished_executions() {
    lock_guard<mutex> lk(executions_mutex);

    auto it = executions.begin();
    while (it != executions.end()) {
        lock_guard<mutex> exec_lk(it->second->mtx);
        const string& status = it->second->status;
        if (status == "completed" || status == "error" || status == "aborted") {
            LOG_INFO("CommandRouterHttpAPI: removing finished execution " << it->first);
            it = executions.erase(it);
        } else {
            ++it;
        }
    }
}

// =============================================================================
// Utilities
// =============================================================================

shared_ptr<CommandRouterHttpAPI::Execution> CommandRouterHttpAPI::find_execution(const string& execution_id) {
    lock_guard<mutex> lk(executions_mutex);
    auto it = executions.find(execution_id);
    return (it != executions.end()) ? it->second : nullptr;
}

string CommandRouterHttpAPI::generate_execution_id() {
    lock_guard<mutex> semaphore(this->executions_mutex);
    ostringstream oss;
    oss << "exec-" << setw(10) << setfill('0') << this->next_execution_number++;
    return oss.str();
}

// =============================================================================
// Event factories
// =============================================================================

JsonConfig CommandRouterHttpAPI::started_event(const string& execution_id) {
    return JsonConfig({{"type", "started"}, {"execution_id", execution_id}});
}

JsonConfig CommandRouterHttpAPI::chunk_event(const string& execution_id,
                                int seq,
                                const vector<string>& data,
                                int received_count) {
    return JsonConfig({{"type", "chunk"},
                      {"execution_id", execution_id},
                      {"seq", seq},
                      {"data", data},
                      {"received_count", received_count}});
}

JsonConfig CommandRouterHttpAPI::completed_event(const string& execution_id, long long duration_ms, int total_items) {
    return JsonConfig({{"type", "completed"},
                      {"execution_id", execution_id},
                      {"duration_ms", duration_ms},
                      {"total_items", total_items}});
}

JsonConfig CommandRouterHttpAPI::error_event(const string& execution_id, const string& message) {
    return JsonConfig({{"type", "error"}, {"execution_id", execution_id}, {"message", message}});
}

JsonConfig CommandRouterHttpAPI::aborted_event(const string& execution_id) {
    return JsonConfig({{"type", "aborted"}, {"execution_id", execution_id}});
}

// =============================================================================
// HTTP response helpers
// =============================================================================

// JsonConfig CommandRouterHttpAPI::make_json(const nlohmann::json& value) { return JsonConfig(value); }

void CommandRouterHttpAPI::set_json_response(httplib::Response& response, int status, const JsonConfig& body) {
    response.status = status;
    response.set_content(static_cast<const json&>(body).dump(), "application/json");
}

// JsonConfig CommandRouterHttpAPI::parse_request_json(const string& body) { return JsonConfig(json::parse(body)); }