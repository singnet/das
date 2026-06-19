#include <nlohmann/json.hpp>

#include "CommandRouterHttpAPI.h"
#include "CommandRouterHttpAPISingleton.h"
#include "DedicatedThread.h"
#include "JsonConfig.h"
#include "Utils.h"
#include "gtest/gtest.h"
#include "httplib.h"
#include "processor/ThreadPool.h"

using namespace command_router;
using namespace processor;
using namespace commons;
using json = nlohmann::json;

static const string TEST_HOST = "localhost";
static const int TEST_PORT = 19001;

class CommandRouterHttpAPITest : public ::testing::Test {
   protected:
    static shared_ptr<ThreadPool> thread_pool;
    static shared_ptr<CommandRouterHttpAPI> api;
    static shared_ptr<DedicatedThread> api_thread;

    static void SetUpTestSuite() {
        thread_pool = make_shared<ThreadPool>("test_thread_pool", 2);
        api = make_shared<CommandRouterHttpAPI>(TEST_HOST, TEST_PORT, thread_pool);
        api_thread = make_shared<DedicatedThread>("test_api_thread", api.get());
        CommandRouterHttpAPI::initialize(api, {thread_pool, api_thread});
        Utils::sleep(300);
    }

    static void TearDownTestSuite() { api->stop(); }

    httplib::Client make_client() {
        httplib::Client client(TEST_HOST, TEST_PORT);
        client.set_connection_timeout(2);
        client.set_read_timeout(2);
        return client;
    }
};

shared_ptr<ThreadPool> CommandRouterHttpAPITest::thread_pool;
shared_ptr<CommandRouterHttpAPI> CommandRouterHttpAPITest::api;
shared_ptr<DedicatedThread> CommandRouterHttpAPITest::api_thread;

// -----------------------------------------------------------------------------
// CommandRouterHttpAPI tests

TEST_F(CommandRouterHttpAPITest, ping_returns_status_header_and_body) {
    auto client = make_client();
    auto res = client.Get("/ping");
    ASSERT_TRUE(res) << "Server did not respond — is port " << TEST_PORT << " free?";
    EXPECT_EQ(res->status, 200);
    EXPECT_NE(res->get_header_value("Content-Type").find("text/plain"), string::npos);
    EXPECT_EQ(res->body, "PONG!");
}

TEST_F(CommandRouterHttpAPITest, ping_is_stable_across_repeated_requests) {
    auto client = make_client();
    for (int i = 0; i < 100; ++i) {
        auto res = client.Get("/ping");
        ASSERT_TRUE(res) << "Request " << i + 1 << " failed";
        EXPECT_EQ(res->status, 200);
        EXPECT_EQ(res->body, "PONG!");
    }
}

TEST_F(CommandRouterHttpAPITest, ping_concurrent_requests) {
    auto client_fn = [this]() {
        auto client = make_client();
        for (int i = 0; i < 100; ++i) {
            auto res = client.Get("/ping");
            EXPECT_TRUE(res);
            EXPECT_EQ(res->status, 200);
            EXPECT_EQ(res->body, "PONG!");
        }
    };

    vector<thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back(client_fn);
    }
    for (auto& th : threads) {
        th.join();
    }
}

TEST_F(CommandRouterHttpAPITest, unregistered_route_returns_404) {
    auto client = make_client();
    auto res = client.Get("/blah");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 404);
}

// -----------------------------------------------------------------------------
// CommandRouterHttpAPISingleton tests

TEST(CommandRouterHttpAPISingleton, provide_then_get_instance_returns_same_ptr) {
    auto pool = make_shared<ThreadPool>("test_pool", 1);
    auto api = make_shared<CommandRouterHttpAPI>("localhost", 19002, pool);

    CommandRouterHttpAPISingleton::provide(api);

    EXPECT_EQ(CommandRouterHttpAPISingleton::get_instance().get(), api.get());
}

TEST(CommandRouterHttpAPISingleton, provide_nullptr_throws) {
    EXPECT_THROW(CommandRouterHttpAPISingleton::provide(nullptr), runtime_error);
}

TEST(CommandRouterHttpAPISingleton, provide_replaces_previous_instance) {
    auto pool1 = make_shared<ThreadPool>("pool1", 1);
    auto api1 = make_shared<CommandRouterHttpAPI>("localhost", 19003, pool1);
    CommandRouterHttpAPISingleton::provide(api1);
    ASSERT_EQ(CommandRouterHttpAPISingleton::get_instance().get(), api1.get());

    auto pool2 = make_shared<ThreadPool>("pool2", 1);
    auto api2 = make_shared<CommandRouterHttpAPI>("localhost", 19004, pool2);
    CommandRouterHttpAPISingleton::provide(api2);

    EXPECT_EQ(CommandRouterHttpAPISingleton::get_instance().get(), api2.get());
    EXPECT_NE(CommandRouterHttpAPISingleton::get_instance().get(), api1.get());
}

TEST(CommandRouterHttpAPISingleton, calling_init_after_provide_throws_already_initialized) {
    auto pool = make_shared<ThreadPool>("double_init_pool", 1);
    auto api = make_shared<CommandRouterHttpAPI>("localhost", 19005, pool);
    CommandRouterHttpAPISingleton::provide(api);

    json raw = {{"http_api_host", "localhost"}, {"http_api_port", 19005}};
    EXPECT_THROW(CommandRouterHttpAPISingleton::init(JsonConfig(raw)), runtime_error);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}