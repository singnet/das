#include <cctype>
#include <mutex>
#include <nlohmann/json.hpp>

#include "CommandExecution.h"
#include "CommandRouterHttpAPI.h"
#include "CommandRouterHttpAPISingleton.h"
#include "DedicatedThread.h"
#include "JsonConfig.h"
#include "gtest/gtest.h"
#include "httplib.h"
#include "processor/ThreadPool.h"

using namespace command_router;
using namespace processor;
using namespace commons;
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

class HttpAPIServerFixture {
   public:
    void start(int port, const HttpAPISettings& settings = {}, unsigned int num_threads = 8) {
        this->thread_pool = make_shared<ThreadPool>("test_thread_pool", num_threads);
        this->api = make_shared<CommandRouterHttpAPI>(TEST_HOST, port, this->thread_pool, settings);
        this->api_thread = make_shared<DedicatedThread>("test_api_thread", this->api.get());
        CommandRouterHttpAPI::initialize(this->api, {this->api_thread, this->thread_pool});
        Utils::sleep(300);
    }

    void stop() {
        if (this->api != nullptr) {
            this->api->stop();
        }
    }

    httplib::Client make_client(int port) const {
        httplib::Client client(TEST_HOST, port);
        client.set_connection_timeout(2);
        client.set_read_timeout(5);
        return client;
    }

   private:
    shared_ptr<ThreadPool> thread_pool;
    shared_ptr<CommandRouterHttpAPI> api;
    shared_ptr<DedicatedThread> api_thread;
};

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

TEST_F(CommandRouterHttpAPILimitsTest, rejects_concurrency_limit) {
    auto first = client().Post("/command-router/executions",
                               make_execution_body("query", SHORT_COMMAND_TEXT).dump(),
                               "application/json");
    ASSERT_TRUE(first);
    EXPECT_EQ(first->status, 202);

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
    EXPECT_THROW(CommandRouterHttpAPISingleton::init(JsonConfig(raw)), runtime_error);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
