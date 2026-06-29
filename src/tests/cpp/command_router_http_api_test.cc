#include <cctype>
#include <mutex>
#include <nlohmann/json.hpp>

#include "AtomDBSingleton.h"
#include "BaseProxy.h"
#include "BaseQueryProxy.h"
#include "BusCommandRouterProcessor.h"
#include "BusCommandRouterProxy.h"
#include "BusCommandRouterProxyStreamPoller.h"
#include "CommandExecution.h"
#include "CommandRouterHttpAPI.h"
#include "CommandRouterHttpAPIConfig.h"
#include "CommandRouterHttpAPISingleton.h"
#include "DedicatedThread.h"
#include "JsonConfig.h"
#include "PortPool.h"
#include "QueryAnswer.h"
#include "ServiceBus.h"
#include "TestAtomDBJsonConfig.h"
#include "TestSystemParams.h"
#include "gtest/gtest.h"
#include "httplib.h"
#include "processor/ThreadPool.h"

using namespace command_router;
using namespace processor;
using namespace commons;
using namespace agents;
using namespace atomdb;
using namespace query_engine;
using namespace service_bus;
using das_test::init_test_system_parameters_singleton;
using json = nlohmann::json;

namespace {

const string TEST_HOST = "localhost";
const int TEST_PORT = 19001;
const int TEST_PORT_LIMITS = 19006;
const string UNKNOWN_EXECUTION_ID = "exec-00000000000000000000000000000000";
const string SHORT_COMMAND_TEXT = "Blah";

json make_execution_body(const string& command_type = "query",
                         const string& command_text = "(Similarity \"human\" %V)") {
    return {{"command_type", command_type}, {"command_text", command_text}};
}

/**
 * HTTP API fixture with an in-process command router bus so executions reach "running".
 */
class HttpAPIServerFixture {
   public:
    void start(int port, const HttpAPISettings& settings = {}, unsigned int num_threads = 8) {
        if (!service_bus_statics_initialized) {
            ServiceBus::initialize_statics({ServiceBus::BUS_COMMAND_ROUTER}, 49500, 49999);
            service_bus_statics_initialized = true;
        }

        const unsigned int router_port = PortPool::get_port();
        const string router_id = TEST_HOST + ":" + std::to_string(router_port);

        this->router_processor = make_shared<BusCommandRouterProcessor>();
        this->router_bus = make_shared<ServiceBus>(router_id);
        this->router_bus->register_processor(this->router_processor);
        Utils::sleep(500);

        this->thread_pool = make_shared<ThreadPool>("test_thread_pool", num_threads);
        this->api = make_shared<CommandRouterHttpAPI>(
            TEST_HOST, port, this->thread_pool, settings, this->router_processor, TEST_HOST);
        this->api_thread = make_shared<DedicatedThread>("test_api_thread", this->api.get());
        CommandRouterHttpAPI::initialize(this->api, {this->api_thread, this->thread_pool});
        Utils::sleep(300);
    }

    void stop() {
        if (this->api != nullptr) {
            this->api->stop();
            this->api = nullptr;
        }
        this->router_processor = nullptr;
        this->router_bus = nullptr;
    }

    httplib::Client make_client(int port) const {
        httplib::Client client(TEST_HOST, port);
        client.set_connection_timeout(2);
        client.set_read_timeout(5);
        return client;
    }

   private:
    inline static bool service_bus_statics_initialized = false;
    shared_ptr<ServiceBus> router_bus;
    shared_ptr<BusCommandRouterProcessor> router_processor;
    shared_ptr<ThreadPool> thread_pool;
    shared_ptr<CommandRouterHttpAPI> api;
    shared_ptr<DedicatedThread> api_thread;
};

class CommandRouterStreamTestEnvironment : public ::testing::Environment {
   public:
    void SetUp() override {
        AtomDBSingleton::init(test_atomdb_json_config());
        init_test_system_parameters_singleton();
    }
};

class StreamTestProxy : public BusCommandRouterProxy {
   public:
    StreamTestProxy() : BusCommandRouterProxy("query", "(Similarity %V1 %V2)") {}

    void enqueue_answers(unsigned int count) {
        vector<string> bundle;
        for (unsigned int i = 0; i < count; ++i) {
            QueryAnswer answer("answer-" + std::to_string(i), 0.0);
            bundle.push_back(answer.tokenize());
        }
        this->from_remote_peer(BaseQueryProxy::ANSWER_BUNDLE, bundle);
    }

    void mark_routed() { this->from_remote_peer(BusCommandRouterProxy::ROUTED, {}); }

    void mark_finished() { this->from_remote_peer(BaseProxy::FINISHED, {}); }
};

size_t total_items(const vector<vector<string>>& chunks) {
    size_t count = 0;
    for (const auto& chunk : chunks) {
        count += chunk.size();
    }
    return count;
}

vector<size_t> chunk_sizes(const vector<vector<string>>& chunks) {
    vector<size_t> sizes;
    for (const auto& chunk : chunks) {
        sizes.push_back(chunk.size());
    }
    return sizes;
}

JsonConfig make_command_router_config(const json& overrides = json::object()) {
    json root = {{"endpoint", "localhost:40008"},
                 {"ports_range", "48000:48999"},
                 {"http_api",
                  {{"endpoint", "localhost:40009"},
                   {"thread_pool_size", 8},
                   {"max_concurrent_executions", 50},
                   {"max_queued_executions", 200},
                   {"max_events_per_execution", 5000},
                   {"stream_items_per_chunk", 25},
                   {"execution_retention_ms", 123456}}}};
    root.merge_patch(overrides);
    return JsonConfig(root);
}

}  // namespace

class CommandRouterHttpAPITest : public ::testing::Test {
   protected:
    static HttpAPIServerFixture server;

    static void SetUpTestSuite() { server.start(TEST_PORT); }
    static void TearDownTestSuite() { server.stop(); }

    httplib::Client client() { return server.make_client(TEST_PORT); }
};

HttpAPIServerFixture CommandRouterHttpAPITest::server;

class CommandRouterHttpAPILimitsTest : public ::testing::Test {
   protected:
    static HttpAPIServerFixture server;

    static void SetUpTestSuite() {
        HttpAPISettings settings;
        settings.max_concurrent_executions = 1;
        server.start(TEST_PORT_LIMITS, settings);
    }

    static void TearDownTestSuite() { server.stop(); }

    httplib::Client client() { return server.make_client(TEST_PORT_LIMITS); }
};

HttpAPIServerFixture CommandRouterHttpAPILimitsTest::server;

class CommandRouterHttpAPIQueuedConcurrencyTest : public ::testing::Test {
   protected:
    static HttpAPIServerFixture server;

    static void SetUpTestSuite() {
        HttpAPISettings settings;
        settings.max_concurrent_executions = 2;
        settings.max_queued_executions = 10;
        server.start(19007, settings, 4);
    }

    static void TearDownTestSuite() { server.stop(); }

    httplib::Client client() { return server.make_client(19007); }
};

HttpAPIServerFixture CommandRouterHttpAPIQueuedConcurrencyTest::server;

class CommandRouterHttpAPISingletonTest : public ::testing::Test {
    void TearDown() override { CommandRouterHttpAPISingleton::provide(nullptr); }
};

// -----------------------------------------------------------------------------
// CommandExecution (no HTTP)

TEST(CommandExecutionTest, status_and_terminal_flags) {
    EXPECT_EQ(CommandExecution::status_to_string(ExecutionStatus::RUNNING), "running");
    EXPECT_TRUE(CommandExecution::is_terminal(ExecutionStatus::COMPLETED));
    EXPECT_FALSE(CommandExecution::is_terminal(ExecutionStatus::RUNNING));
}

TEST(CommandExecutionTest, terminal_marks_finished_at) {
    CommandExecution exec("exec-abc", "query", "(Similarity %V1 %V2)");

    lock_guard<mutex> lock(exec.mtx);
    exec.mark_completed(100, 5);
    EXPECT_GT(exec.finished_at_ms, 0);
}

TEST(CommandExecutionTest, event_buffer_overflow_raises) {
    CommandExecution exec("exec-abc", "query", "(Similarity %V1 %V2)", 2);

    lock_guard<mutex> lock(exec.mtx);
    exec.mark_running();
    exec.publish_chunk(1, {"a"});
    EXPECT_THROW(exec.publish_chunk(2, {"b", "c"}), runtime_error);
}

// -----------------------------------------------------------------------------
// CommandRouterHttpAPIConfig

TEST(CommandRouterHttpAPIConfigTest, from_config_loads_http_api_fields) {
    const auto config = CommandRouterHttpAPIConfig::from_config(make_command_router_config());

    EXPECT_EQ(config.host, "localhost");
    EXPECT_EQ(config.port, 40009);
    EXPECT_EQ(config.thread_pool_size, 8u);
    EXPECT_EQ(config.bus_host, "localhost");
    EXPECT_EQ(config.settings.max_concurrent_executions, 50u);
    EXPECT_EQ(config.settings.max_queued_executions, 200u);
    EXPECT_EQ(config.settings.max_events_per_execution, 5000u);
    EXPECT_EQ(config.settings.execution_retention_ms, 123456);
    EXPECT_EQ(config.settings.stream_items_per_chunk, 25u);
}

TEST(CommandRouterHttpAPIConfigTest, from_config_rejects_zero_stream_items_per_chunk) {
    EXPECT_THROW(CommandRouterHttpAPIConfig::from_config(
                     make_command_router_config({{"http_api", {{"stream_items_per_chunk", 0}}}})),
                 runtime_error);
}

TEST(CommandRouterHttpAPIConfigTest, from_config_rejects_invalid_http_api_endpoint) {
    EXPECT_THROW(CommandRouterHttpAPIConfig::from_config(
                     make_command_router_config({{"http_api", {{"endpoint", "not-a-valid-endpoint"}}}})),
                 runtime_error);
}

// -----------------------------------------------------------------------------
// BusCommandRouterProcessor HTTP dispatch

TEST(BusCommandRouterProcessorTest, dispatch_http_command_get_returns_params) {
    set<string> commands = {ServiceBus::BUS_COMMAND_ROUTER};
    ServiceBus::initialize_statics(commands, 49400, 49499);

    auto router_processor = make_shared<BusCommandRouterProcessor>();
    const string router_id = TEST_HOST + ":" + std::to_string(PortPool::get_port());
    ServiceBus router_bus(router_id);
    router_bus.register_processor(router_processor);
    Utils::sleep(500);

    auto caller_proxy = make_shared<BusCommandRouterProxy>("get", "params");
    router_processor->dispatch_http_command(caller_proxy, "exec-http-get-test");
    Utils::sleep(500);

    EXPECT_FALSE(caller_proxy->params_response.empty());
    EXPECT_TRUE(caller_proxy->finished());
}

// -----------------------------------------------------------------------------
// BusCommandRouterProxyStreamPoller (HTTP stream polling)

TEST(BusCommandRouterProxyStreamPollerTest, stream_emits_one_item_per_chunk_by_default) {
    auto proxy = make_shared<StreamTestProxy>();
    proxy->mark_routed();
    proxy->enqueue_answers(5);
    proxy->mark_finished();

    vector<vector<string>> chunks;
    auto on_chunk = [&](const vector<string>& chunk) { chunks.push_back(chunk); };

    ASSERT_TRUE(BusCommandRouterProxyStreamPoller::poll_stream(
        proxy, "query", 1, nullptr, on_chunk, nullptr, nullptr));
    EXPECT_EQ(chunks.size(), 5u);
    EXPECT_EQ(chunk_sizes(chunks), (vector<size_t>{1, 1, 1, 1, 1}));
    EXPECT_EQ(total_items(chunks), 5u);
    EXPECT_NE(chunks.front().front().find("answer-0"), string::npos);
}

TEST(BusCommandRouterProxyStreamPollerTest, stream_groups_items_per_chunk) {
    auto proxy = make_shared<StreamTestProxy>();
    proxy->mark_routed();
    proxy->enqueue_answers(7);
    proxy->mark_finished();

    vector<vector<string>> chunks;
    auto on_chunk = [&](const vector<string>& chunk) { chunks.push_back(chunk); };

    ASSERT_TRUE(BusCommandRouterProxyStreamPoller::poll_stream(
        proxy, "query", 3, nullptr, on_chunk, nullptr, nullptr));
    EXPECT_EQ(chunks.size(), 3u);
    EXPECT_EQ(chunk_sizes(chunks), (vector<size_t>{3, 3, 1}));
    EXPECT_EQ(total_items(chunks), 7u);
}

TEST(BusCommandRouterProxyStreamPollerTest, rejects_zero_items_per_chunk) {
    auto proxy = make_shared<StreamTestProxy>();
    proxy->mark_routed();
    proxy->enqueue_answers(1);
    proxy->mark_finished();

    string error_message;
    auto on_error = [&](const string& message) { error_message = message; };

    EXPECT_FALSE(BusCommandRouterProxyStreamPoller::poll_stream(
        proxy, "query", 0, nullptr, nullptr, on_error, nullptr));
    EXPECT_NE(error_message.find("items_per_chunk"), string::npos);
}

TEST(BusCommandRouterProxyStreamPollerTest, get_and_set_emit_single_chunk) {
    auto get_proxy = make_shared<StreamTestProxy>();
    get_proxy->from_remote_peer(BusCommandRouterProxy::PARAMS_RESPONSE, {"params-body"});

    vector<vector<string>> get_chunks;
    ASSERT_TRUE(BusCommandRouterProxyStreamPoller::poll_stream(
        get_proxy,
        "get",
        1,
        nullptr,
        [&](const vector<string>& chunk) { get_chunks.push_back(chunk); },
        nullptr,
        nullptr));
    ASSERT_EQ(get_chunks.size(), 1u);
    EXPECT_EQ(get_chunks[0], vector<string>{"params-body"});

    auto set_proxy = make_shared<StreamTestProxy>();
    set_proxy->from_remote_peer(BusCommandRouterProxy::SET_PARAM_ACK, {"ack-body"});

    vector<vector<string>> set_chunks;
    ASSERT_TRUE(BusCommandRouterProxyStreamPoller::poll_stream(
        set_proxy,
        "set",
        1,
        nullptr,
        [&](const vector<string>& chunk) { set_chunks.push_back(chunk); },
        nullptr,
        nullptr));
    ASSERT_EQ(set_chunks.size(), 1u);
    EXPECT_EQ(set_chunks[0], vector<string>{"ack-body"});
}

// -----------------------------------------------------------------------------
// HTTP API

TEST_F(CommandRouterHttpAPITest, ping_and_unknown_route) {
    auto res_ping = client().Get("/ping");
    ASSERT_TRUE(res_ping);
    EXPECT_EQ(res_ping->status, 200);
    EXPECT_EQ(res_ping->body, "PONG!");

    auto res_unknown = client().Get("/blah");
    ASSERT_TRUE(res_unknown);
    EXPECT_EQ(res_unknown->status, 404);
}

TEST_F(CommandRouterHttpAPITest, create_execution_returns_202) {
    auto res =
        client().Post("/command-router/executions", make_execution_body().dump(), "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 202);

    auto payload = json::parse(res->body);
    EXPECT_EQ(payload["status"], "pending");
}

TEST_F(CommandRouterHttpAPITest, create_execution_rejects_invalid_requests) {
    auto bad_json = client().Post("/command-router/executions", "{bad", "application/json");
    ASSERT_TRUE(bad_json);
    EXPECT_EQ(bad_json->status, 400);

    auto missing_field =
        client().Post("/command-router/executions", R"({"command_type":"query"})", "application/json");
    ASSERT_TRUE(missing_field);
    EXPECT_EQ(missing_field->status, 400);

    auto bad_type = client().Post(
        "/command-router/executions", make_execution_body("invalid", "arg").dump(), "application/json");
    ASSERT_TRUE(bad_type);
    EXPECT_EQ(bad_type->status, 400);
}

TEST_F(CommandRouterHttpAPITest, get_execution_reports_running_then_unknown_returns_404) {
    auto create =
        client().Post("/command-router/executions", make_execution_body().dump(), "application/json");
    ASSERT_TRUE(create);
    ASSERT_EQ(create->status, 202);

    const auto execution_id = json::parse(create->body)["execution_id"].get<string>();

    string status;
    for (int attempt = 0; attempt < 20; ++attempt) {
        auto get_res = client().Get("/command-router/executions/" + execution_id);
        ASSERT_TRUE(get_res);
        ASSERT_EQ(get_res->status, 200);
        status = json::parse(get_res->body)["status"].get<string>();
        if (status == "running") {
            break;
        }
        Utils::sleep(100);
    }
    EXPECT_EQ(status, "running");

    auto unknown = client().Get("/command-router/executions/" + UNKNOWN_EXECUTION_ID);
    ASSERT_TRUE(unknown);
    EXPECT_EQ(unknown->status, 404);
}

TEST_F(CommandRouterHttpAPITest, cancel_running_execution_aborts_and_second_cancel_returns_409) {
    auto create =
        client().Post("/command-router/executions", make_execution_body().dump(), "application/json");
    ASSERT_TRUE(create);
    ASSERT_EQ(create->status, 202);

    const auto execution_id = json::parse(create->body)["execution_id"].get<string>();

    string status;
    for (int attempt = 0; attempt < 50; ++attempt) {
        auto get_res = client().Get("/command-router/executions/" + execution_id);
        ASSERT_TRUE(get_res);
        ASSERT_EQ(get_res->status, 200);
        status = json::parse(get_res->body)["status"].get<string>();
        if (status == "running") {
            break;
        }
        Utils::sleep(100);
    }
    ASSERT_EQ(status, "running");

    auto cancel =
        client().Post("/command-router/executions/" + execution_id + "/cancel", "", "application/json");
    ASSERT_TRUE(cancel);
    EXPECT_EQ(cancel->status, 200);

    for (int attempt = 0; attempt < 30; ++attempt) {
        auto get_res = client().Get("/command-router/executions/" + execution_id);
        ASSERT_TRUE(get_res);
        ASSERT_EQ(get_res->status, 200);
        status = json::parse(get_res->body)["status"].get<string>();
        if (status == "aborted") {
            break;
        }
        Utils::sleep(100);
    }
    EXPECT_EQ(status, "aborted");

    auto second_cancel =
        client().Post("/command-router/executions/" + execution_id + "/cancel", "", "application/json");
    ASSERT_TRUE(second_cancel);
    EXPECT_EQ(second_cancel->status, 409);
    EXPECT_EQ(json::parse(second_cancel->body)["status"], "aborted");
}

TEST_F(CommandRouterHttpAPIQueuedConcurrencyTest, queued_executions_respect_concurrent_limit) {
    vector<string> execution_ids;
    for (int i = 0; i < 4; ++i) {
        auto create =
            client().Post("/command-router/executions",
                          make_execution_body("query", SHORT_COMMAND_TEXT + std::to_string(i)).dump(),
                          "application/json");
        ASSERT_TRUE(create);
        if (create->status == 202) {
            execution_ids.push_back(json::parse(create->body)["execution_id"].get<string>());
        }
    }
    ASSERT_GE(execution_ids.size(), 2u);

    int max_observed_running = 0;
    for (int attempt = 0; attempt < 100; ++attempt) {
        int running_count = 0;
        for (const auto& execution_id : execution_ids) {
            auto get_res = client().Get("/command-router/executions/" + execution_id);
            ASSERT_TRUE(get_res);
            ASSERT_EQ(get_res->status, 200);
            if (json::parse(get_res->body)["status"].get<string>() == "running") {
                ++running_count;
            }
        }
        max_observed_running = std::max(max_observed_running, running_count);
        Utils::sleep(50);
    }

    EXPECT_LE(max_observed_running, 2);
}

TEST_F(CommandRouterHttpAPILimitsTest, rejects_concurrency_limit) {
    auto first = client().Post("/command-router/executions",
                               make_execution_body("query", SHORT_COMMAND_TEXT).dump(),
                               "application/json");
    ASSERT_TRUE(first);
    ASSERT_EQ(first->status, 202);

    const auto execution_id = json::parse(first->body)["execution_id"].get<string>();

    string status;
    for (int attempt = 0; attempt < 50; ++attempt) {
        auto get_res = client().Get("/command-router/executions/" + execution_id);
        ASSERT_TRUE(get_res);
        ASSERT_EQ(get_res->status, 200);
        status = json::parse(get_res->body)["status"].get<string>();
        if (status == "running") {
            break;
        }
        Utils::sleep(100);
    }
    ASSERT_EQ(status, "running");

    auto second = client().Post("/command-router/executions",
                                make_execution_body("evolution", SHORT_COMMAND_TEXT).dump(),
                                "application/json");
    ASSERT_TRUE(second);
    EXPECT_EQ(second->status, 429);
}

TEST_F(CommandRouterHttpAPITest, websocket_streams_lifecycle_events) {
    {
        httplib::ws::WebSocketClient ws("ws://" + TEST_HOST + ":" + std::to_string(TEST_PORT) +
                                        "/command-router/ws/" + UNKNOWN_EXECUTION_ID);
        ASSERT_TRUE(ws.is_valid());
        ASSERT_TRUE(ws.connect());

        string msg;
        EXPECT_FALSE(ws.read(msg));
    }

    auto create =
        client().Post("/command-router/executions", make_execution_body().dump(), "application/json");
    ASSERT_TRUE(create);
    ASSERT_EQ(create->status, 202);

    const auto execution_id = json::parse(create->body)["execution_id"].get<string>();

    httplib::ws::WebSocketClient ws("ws://" + TEST_HOST + ":" + std::to_string(TEST_PORT) +
                                    "/command-router/ws/" + execution_id);
    ASSERT_TRUE(ws.is_valid());
    ASSERT_TRUE(ws.connect());
    ws.set_read_timeout(5, 0);

    bool saw_running = false;
    string msg;
    while (ws.read(msg)) {
        auto event = json::parse(msg);
        EXPECT_EQ(event["execution_id"].get<string>(), execution_id);
        if (event.value("status", "") == "running") {
            saw_running = true;
            break;
        }
    }
    ws.close();
    EXPECT_TRUE(saw_running);

    client().Post("/command-router/executions/" + execution_id + "/cancel", "", "application/json");
}

// -----------------------------------------------------------------------------
// Singleton

TEST_F(CommandRouterHttpAPISingletonTest, provide_and_get_instance) {
    auto pool = make_shared<ThreadPool>("test_pool", 1);
    auto api = make_shared<CommandRouterHttpAPI>("localhost", 19002, pool);
    CommandRouterHttpAPISingleton::provide(api);
    EXPECT_EQ(CommandRouterHttpAPISingleton::get_instance().get(), api.get());
}

TEST_F(CommandRouterHttpAPISingletonTest, init_after_provide_throws) {
    auto pool = make_shared<ThreadPool>("double_init_pool", 1);
    CommandRouterHttpAPISingleton::provide(make_shared<CommandRouterHttpAPI>("localhost", 19005, pool));

    json raw = {{"http_api", {{"endpoint", "localhost:19005"}}}};
    EXPECT_THROW(
        CommandRouterHttpAPISingleton::init(JsonConfig(raw), make_shared<BusCommandRouterProcessor>()),
        runtime_error);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new CommandRouterStreamTestEnvironment());
    return RUN_ALL_TESTS();
}
