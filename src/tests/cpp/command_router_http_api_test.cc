#include <nlohmann/json.hpp>
#include <thread>

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
#include "HttpCommandProxyFactory.h"
#include "JsonConfig.h"
#include "PatternMatchingQueryProxy.h"
#include "PortPool.h"
#include "QueryAnswer.h"
#include "ServiceBus.h"
#include "TestAtomDBJsonConfig.h"
#include "TestSystemParams.h"
#include "expression_hasher.h"
#include "gtest/gtest.h"
#include "httplib.h"
#include "processor/ThreadPool.h"

using namespace command_router;
using namespace processor;
using namespace commons;
using namespace atomdb;
using namespace query_engine;
using namespace service_bus;
using das_test::init_test_system_parameters_singleton;
using json = nlohmann::json;

namespace {

const string TEST_HOST = "localhost";
const int TEST_PORT = 19001;
const int TEST_PORT_THREAD_POOL = 19007;
const int TEST_PORT_PARALLEL = 19008;
const string UNKNOWN_EXECUTION_ID = "exec-00000000000000000000000000000000";
const string SHORT_COMMAND_TEXT = "Blah";

json make_execution_body(const string& command = "query",
                         const string& query_token = "(Similarity \"human\" %V)") {
    return {{"command", command},
            {"params", {{"query", {{"syntax", "metta"}, {"tokens", json::array({query_token})}}}}}};
}

string hash_string(const string& input) {
    char* hash = compute_hash(const_cast<char*>(input.c_str()));
    string result(hash);
    delete[] hash;
    return result;
}

class HangingQueryForwardProxy : public BusCommandProxy {
   public:
    void pack_command_line_args() override {}
};

/** Accepts forwarded queries but never responds, so HTTP executions stay running. */
class HangingQueryForwardProcessor : public BusCommandProcessor {
   public:
    HangingQueryForwardProcessor() : BusCommandProcessor({ServiceBus::PATTERN_MATCHING_QUERY}) {}

    shared_ptr<BusCommandProxy> factory_empty_proxy() override {
        return make_shared<HangingQueryForwardProxy>();
    }

    void run_command(shared_ptr<BusCommandProxy> /*proxy*/) override {}
};

/** Replies with one answer whose handle is hash(query_tokens), plus max_answers-1 extras. */
class EchoQueryProcessor : public BusCommandProcessor {
   public:
    EchoQueryProcessor() : BusCommandProcessor({ServiceBus::PATTERN_MATCHING_QUERY}) {}

    shared_ptr<BusCommandProxy> factory_empty_proxy() override {
        return make_shared<PatternMatchingQueryProxy>();
    }

    void run_command(shared_ptr<BusCommandProxy> proxy) override {
        auto query = dynamic_pointer_cast<PatternMatchingQueryProxy>(proxy);
        if (query == nullptr) {
            return;
        }
        // Bus delivers packed args; real processors untokenize before reading tokens/params.
        query->untokenize(query->args);
        const string query_key = Utils::join(query->get_query_tokens(), ' ');
        unsigned int n = query->parameters.get<unsigned int>(BaseQueryProxy::MAX_ANSWERS);
        if (n == 0) {
            n = 1;
        }
        std::thread([query, query_key, n]() {
            Utils::sleep(5 + (query->get_serial() % 30));
            for (unsigned int i = 0; i < n; ++i) {
                query->push(
                    make_shared<QueryAnswer>(hash_string(query_key + "#" + std::to_string(i)), 0.0));
            }
            query->query_processing_finished();
        }).detach();
    }
};

void initialize_test_service_bus_statics_once() {
    static bool initialized = false;
    if (!initialized) {
        ServiceBus::initialize_statics(
            {ServiceBus::BUS_COMMAND_ROUTER, ServiceBus::PATTERN_MATCHING_QUERY}, 49400, 49999);
        initialized = true;
    }
}

/**
 * HTTP API fixture with an in-process command router bus so executions reach "running".
 */
class HttpAPIServerFixture {
   public:
    void start(int port,
               const HttpAPISettings& settings = {},
               unsigned int num_threads = 8,
               shared_ptr<BusCommandProcessor> query_processor = nullptr) {
        initialize_test_service_bus_statics_once();

        const unsigned int query_port = PortPool::get_port();
        const string query_id = TEST_HOST + ":" + std::to_string(query_port);
        const unsigned int router_port = PortPool::get_port();
        const string router_id = TEST_HOST + ":" + std::to_string(router_port);

        if (query_processor == nullptr) {
            query_processor = make_shared<HangingQueryForwardProcessor>();
        }

        this->query_bus = make_shared<ServiceBus>(query_id);
        this->query_bus->register_processor(query_processor);
        Utils::sleep(300);

        this->router_bus = make_shared<ServiceBus>(router_id, query_id);
        this->router_processor = make_shared<BusCommandRouterProcessor>(this->router_bus);
        this->router_bus->register_processor(this->router_processor);
        Utils::sleep(500);

        this->thread_pool = make_shared<ThreadPool>("test_thread_pool", num_threads);
        this->api = make_shared<CommandRouterHttpAPI>(
            TEST_HOST, port, this->thread_pool, this->router_processor, settings, TEST_HOST);
        this->api_thread = make_shared<DedicatedThread>("test_api_thread", this->api.get());
        CommandRouterHttpAPI::initialize(this->api, {this->api_thread, this->thread_pool});
        Utils::sleep(300);
    }

    void stop() {
        if (this->api != nullptr) {
            this->api->stop();
        }
        this->api_thread = nullptr;
        this->thread_pool = nullptr;
        this->api = nullptr;
        this->router_processor = nullptr;
        this->router_bus = nullptr;
        this->query_bus = nullptr;
    }

    httplib::Client make_client(int port) const {
        httplib::Client client(TEST_HOST, port);
        client.set_connection_timeout(2);
        client.set_read_timeout(15);
        return client;
    }

   private:
    shared_ptr<ServiceBus> query_bus;
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

size_t total_items(const vector<json>& chunks) {
    size_t count = 0;
    for (const auto& chunk : chunks) {
        count += chunk.size();
    }
    return count;
}

vector<size_t> chunk_sizes(const vector<json>& chunks) {
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

class CommandRouterHttpAPIThreadPoolConcurrencyTest : public ::testing::Test {
   protected:
    static constexpr unsigned int kThreadPoolSize = 2;
    static HttpAPIServerFixture server;

    static void SetUpTestSuite() {
        HttpAPISettings settings;
        settings.max_queued_executions = 10;
        server.start(TEST_PORT_THREAD_POOL, settings, kThreadPoolSize);
    }

    static void TearDownTestSuite() { server.stop(); }

    httplib::Client client() { return server.make_client(TEST_PORT_THREAD_POOL); }
};

HttpAPIServerFixture CommandRouterHttpAPIThreadPoolConcurrencyTest::server;

class CommandRouterHttpAPIParallelTest : public ::testing::Test {
   protected:
    static HttpAPIServerFixture server;

    static void SetUpTestSuite() {
        server.start(TEST_PORT_PARALLEL, {}, 8, make_shared<EchoQueryProcessor>());
    }

    static void TearDownTestSuite() { server.stop(); }
};

HttpAPIServerFixture CommandRouterHttpAPIParallelTest::server;

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
    CommandExecution exec(
        "exec-abc",
        "query",
        {{"query", {{"syntax", "metta"}, {"tokens", json::array({"(Similarity %V1 %V2)"})}}}});

    exec.mark_completed(100, 5);
    EXPECT_GT(exec.finished_at_ms(), 0);
}

TEST(CommandExecutionTest, event_buffer_overflow_raises) {
    CommandExecution exec(
        "exec-abc",
        "query",
        {{"query", {{"syntax", "metta"}, {"tokens", json::array({"(Similarity %V1 %V2)"})}}}},
        2);

    exec.mark_running();
    exec.publish_chunk(1, json::array({json("a")}));
    EXPECT_THROW(exec.publish_chunk(2, json::array({json("b"), json("c")})), runtime_error);
}

// -----------------------------------------------------------------------------
// CommandRouterHttpAPIConfig

TEST(CommandRouterHttpAPIConfigTest, from_config_loads_http_api_fields) {
    const auto config = CommandRouterHttpAPIConfig::from_config(make_command_router_config());

    EXPECT_EQ(config.host, "localhost");
    EXPECT_EQ(config.port, 40009);
    EXPECT_EQ(config.thread_pool_size, 8u);
    EXPECT_EQ(config.bus_host, "localhost");
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

TEST(CommandRouterHttpAPIConfigTest, from_config_rejects_non_numeric_http_api_port) {
    EXPECT_THROW(CommandRouterHttpAPIConfig::from_config(
                     make_command_router_config({{"http_api", {{"endpoint", "localhost:abc"}}}})),
                 runtime_error);
}

TEST(CommandRouterHttpAPIConfigTest, from_config_rejects_trailing_junk_http_api_port) {
    EXPECT_THROW(CommandRouterHttpAPIConfig::from_config(
                     make_command_router_config({{"http_api", {{"endpoint", "localhost:8080abc"}}}})),
                 runtime_error);
}

// -----------------------------------------------------------------------------
// BusCommandRouterProcessor HTTP dispatch

TEST(BusCommandRouterProcessorTest, dispatch_http_command_get_returns_params) {
    initialize_test_service_bus_statics_once();

    const string router_id = TEST_HOST + ":" + std::to_string(PortPool::get_port());
    auto router_bus = make_shared<ServiceBus>(router_id);
    auto router_processor = make_shared<BusCommandRouterProcessor>(router_bus);
    router_bus->register_processor(router_processor);
    Utils::sleep(500);

    auto caller_proxy = make_shared<BusCommandRouterProxy>("get", "params");
    router_processor->dispatch_http_command(caller_proxy, TEST_HOST + ":http-get-test");
    Utils::sleep(500);

    EXPECT_FALSE(caller_proxy->params_response.empty());
    EXPECT_TRUE(caller_proxy->finished());
}

TEST(BusCommandRouterProcessorTest, dispatch_http_command_preserves_caller_parameters) {
    initialize_test_service_bus_statics_once();

    const string requestor_id = TEST_HOST + ":http-param-preserve-test";
    const string router_id = TEST_HOST + ":" + std::to_string(PortPool::get_port());
    auto router_bus = make_shared<ServiceBus>(router_id);
    auto router_processor = make_shared<BusCommandRouterProcessor>(router_bus);
    router_bus->register_processor(router_processor);
    Utils::sleep(500);

    // Peer store would have use_metta_as_query_tokens=true if HTTP still synced from it.
    auto set_caller = make_shared<BusCommandRouterProxy>("set", "param use_metta_as_query_tokens true");
    router_processor->dispatch_http_command(set_caller, requestor_id);
    Utils::sleep(500);

    auto query_caller = make_shared<BusCommandRouterProxy>("query", "(Similarity %V1 %V2)");
    query_caller->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] = false;
    query_caller->parameters[BaseQueryProxy::MAX_ANSWERS] = (unsigned int) 10;
    router_processor->dispatch_http_command(query_caller, requestor_id);

    EXPECT_FALSE(query_caller->parameters.get<bool>(BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS));
    EXPECT_EQ(query_caller->parameters.get<unsigned int>(BaseQueryProxy::MAX_ANSWERS), 10u);
}

TEST(HttpCommandProxyFactoryTest, create_query_sets_params_onto_proxy_defaults) {
    string error;
    const json params = {
        {"query", {{"syntax", "metta"}, {"tokens", json::array({"(Similarity \"human\" %C)"})}}},
        {"populate_metta_mapping", true},
        {"use_metta_as_query_tokens", "true"},
        {"max_answers", 7}};

    auto proxy = HttpCommandProxyFactory::create(HttpCommandProxyFactory::QUERY, params, error);
    ASSERT_NE(proxy, nullptr) << error;
    EXPECT_EQ(proxy->get_args()[0], "query");
    EXPECT_EQ(proxy->get_args()[1], "(Similarity \"human\" $C)");
    EXPECT_TRUE(proxy->parameters.get<bool>(BaseQueryProxy::POPULATE_METTA_MAPPING));
    EXPECT_TRUE(proxy->parameters.get<bool>(BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS));
    EXPECT_EQ(proxy->parameters.get<unsigned int>(BaseQueryProxy::MAX_ANSWERS), 7u);
}

TEST(HttpCommandProxyFactoryTest, create_rejects_unknown_parameter) {
    string error;
    const json params = {
        {"query", {{"syntax", "metta"}, {"tokens", json::array({"(Similarity \"human\" %C)"})}}},
        {"unknown_key", true}};

    auto proxy = HttpCommandProxyFactory::create(HttpCommandProxyFactory::QUERY, params, error);
    EXPECT_EQ(proxy, nullptr);
    EXPECT_NE(error.find("Unknown parameter"), string::npos);
}

TEST(HttpCommandProxyFactoryTest, create_rejects_unsupported_command) {
    string error;
    auto proxy = HttpCommandProxyFactory::create("evolution", json::object(), error);
    EXPECT_EQ(proxy, nullptr);
    EXPECT_NE(error.find("Unsupported command"), string::npos);
}

// -----------------------------------------------------------------------------
// BusCommandRouterProxyStreamPoller (HTTP stream polling)

TEST(BusCommandRouterProxyStreamPollerTest, stream_emits_one_item_per_chunk_by_default) {
    auto proxy = make_shared<StreamTestProxy>();
    proxy->mark_routed();
    proxy->enqueue_answers(5);
    proxy->mark_finished();

    vector<json> chunks;
    auto on_chunk = [&](const json& chunk) { chunks.push_back(chunk); };

    const auto poll_result = BusCommandRouterProxyStreamPoller::poll_stream(
        proxy, "query", 1, nullptr, on_chunk, nullptr, nullptr);
    ASSERT_TRUE(poll_result.ok);
    EXPECT_FALSE(poll_result.is_count_only);
    EXPECT_EQ(chunks.size(), 5u);
    EXPECT_EQ(chunk_sizes(chunks), (vector<size_t>{1, 1, 1, 1, 1}));
    EXPECT_EQ(total_items(chunks), 5u);
    EXPECT_EQ(chunks.front()[0]["handles"][0][0], "answer-0");
}

TEST(BusCommandRouterProxyStreamPollerTest,
     streams_handles_when_use_metta_true_but_populate_metta_mapping_false) {
    auto proxy = make_shared<StreamTestProxy>();
    proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] = true;
    proxy->parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] = false;
    proxy->mark_routed();

    QueryAnswer answer("7ec8526b8c8f15a6ac55273fedbf694f", 0.0);
    answer.assignment.assign("C", "181a19436acef495c8039a610be59603");
    vector<string> bundle = {answer.tokenize()};
    proxy->from_remote_peer(BaseQueryProxy::ANSWER_BUNDLE, bundle);
    proxy->mark_finished();

    vector<json> chunks;
    const auto poll_result = BusCommandRouterProxyStreamPoller::poll_stream(
        proxy,
        "query",
        1,
        nullptr,
        [&](const json& chunk) { chunks.push_back(chunk); },
        nullptr,
        nullptr);
    ASSERT_TRUE(poll_result.ok);
    ASSERT_EQ(chunks.size(), 1u);
    EXPECT_EQ(chunks[0][0]["handles"][0][0], "7ec8526b8c8f15a6ac55273fedbf694f");
    EXPECT_EQ(chunks[0][0]["assignment"]["C"], "181a19436acef495c8039a610be59603");
}

TEST(BusCommandRouterProxyTest, count_command_sets_total_and_marks_count_received) {
    auto proxy = make_shared<BusCommandRouterProxy>("query", "(Similarity $P $C)");

    ASSERT_TRUE(proxy->from_remote_peer(PatternMatchingQueryProxy::COUNT, {"14"}));
    EXPECT_EQ(proxy->get_count(), 14u);
    EXPECT_TRUE(proxy->count_received);
}

TEST(BusCommandRouterProxyTest, count_command_rejects_negative_and_invalid_count) {
    auto negative_proxy = make_shared<BusCommandRouterProxy>("query", "(Similarity $P $C)");
    EXPECT_THROW(negative_proxy->from_remote_peer(PatternMatchingQueryProxy::COUNT, {"-1"}),
                 runtime_error);

    auto invalid_proxy = make_shared<BusCommandRouterProxy>("query", "(Similarity $P $C)");
    EXPECT_THROW(invalid_proxy->from_remote_peer(PatternMatchingQueryProxy::COUNT, {"abc"}),
                 invalid_argument);
}

TEST(BusCommandRouterProxyStreamPollerTest, count_only_emits_total) {
    auto proxy = make_shared<StreamTestProxy>();
    proxy->mark_routed();
    proxy->from_remote_peer(PatternMatchingQueryProxy::COUNT, {"14"});
    proxy->mark_finished();

    vector<json> chunks;
    const auto poll_result = BusCommandRouterProxyStreamPoller::poll_stream(
        proxy,
        "query",
        1,
        nullptr,
        [&](const json& chunk) { chunks.push_back(chunk); },
        nullptr,
        nullptr);
    ASSERT_TRUE(poll_result.ok);
    EXPECT_TRUE(poll_result.is_count_only);
    EXPECT_EQ(poll_result.count_only_total, 14);
    ASSERT_EQ(chunks.size(), 1u);
    EXPECT_EQ(chunks[0], json::array({json("14")}));
}

TEST(BusCommandRouterProxyStreamPollerTest, streams_answers_when_count_flag_true_but_answers_arrive) {
    auto proxy = make_shared<StreamTestProxy>();
    proxy->parameters[PatternMatchingQueryProxy::COUNT_FLAG] = true;
    proxy->mark_routed();
    proxy->enqueue_answers(3);
    proxy->mark_finished();

    vector<json> chunks;
    const auto poll_result = BusCommandRouterProxyStreamPoller::poll_stream(
        proxy,
        "query",
        1,
        nullptr,
        [&](const json& chunk) { chunks.push_back(chunk); },
        nullptr,
        nullptr);
    ASSERT_TRUE(poll_result.ok);
    EXPECT_FALSE(poll_result.is_count_only);
    EXPECT_EQ(chunks.size(), 3u);
    EXPECT_EQ(total_items(chunks), 3u);
}

TEST(BusCommandRouterProxyStreamPollerTest, stream_groups_items_per_chunk) {
    auto proxy = make_shared<StreamTestProxy>();
    proxy->mark_routed();
    proxy->enqueue_answers(7);
    proxy->mark_finished();

    vector<json> chunks;
    auto on_chunk = [&](const json& chunk) { chunks.push_back(chunk); };

    const auto poll_result = BusCommandRouterProxyStreamPoller::poll_stream(
        proxy, "query", 3, nullptr, on_chunk, nullptr, nullptr);
    ASSERT_TRUE(poll_result.ok);
    EXPECT_FALSE(poll_result.is_count_only);
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

    const auto poll_result = BusCommandRouterProxyStreamPoller::poll_stream(
        proxy, "query", 0, nullptr, nullptr, on_error, nullptr);
    EXPECT_FALSE(poll_result.ok);
    EXPECT_NE(error_message.find("items_per_chunk"), string::npos);
}

TEST(BusCommandRouterProxyStreamPollerTest, get_and_set_emit_single_chunk) {
    auto get_proxy = make_shared<StreamTestProxy>();
    get_proxy->from_remote_peer(BusCommandRouterProxy::PARAMS_RESPONSE, {"params-body"});

    vector<json> get_chunks;
    const auto get_result = BusCommandRouterProxyStreamPoller::poll_stream(
        get_proxy,
        "get",
        1,
        nullptr,
        [&](const json& chunk) { get_chunks.push_back(chunk); },
        nullptr,
        nullptr);
    ASSERT_TRUE(get_result.ok);
    EXPECT_FALSE(get_result.is_count_only);
    ASSERT_EQ(get_chunks.size(), 1u);
    EXPECT_EQ(get_chunks[0], json::array({json("params-body")}));

    auto set_proxy = make_shared<StreamTestProxy>();
    set_proxy->from_remote_peer(BusCommandRouterProxy::SET_PARAM_ACK, {"ack-body"});

    vector<json> set_chunks;
    const auto set_result = BusCommandRouterProxyStreamPoller::poll_stream(
        set_proxy,
        "set",
        1,
        nullptr,
        [&](const json& chunk) { set_chunks.push_back(chunk); },
        nullptr,
        nullptr);
    ASSERT_TRUE(set_result.ok);
    EXPECT_FALSE(set_result.is_count_only);
    ASSERT_EQ(set_chunks.size(), 1u);
    EXPECT_EQ(set_chunks[0], json::array({json("ack-body")}));
}

TEST(BusCommandRouterProxyStreamPollerTest, get_and_set_reject_empty_response) {
    auto get_proxy = make_shared<StreamTestProxy>();
    get_proxy->mark_finished();

    string get_error;
    const auto get_result = BusCommandRouterProxyStreamPoller::poll_stream(
        get_proxy, "get", 1, nullptr, nullptr, [&](const string& msg) { get_error = msg; }, nullptr);
    EXPECT_FALSE(get_result.ok);
    EXPECT_NE(get_error.find("params response"), string::npos);

    auto set_proxy = make_shared<StreamTestProxy>();
    set_proxy->mark_finished();

    string set_error;
    const auto set_result = BusCommandRouterProxyStreamPoller::poll_stream(
        set_proxy, "set", 1, nullptr, nullptr, [&](const string& msg) { set_error = msg; }, nullptr);
    EXPECT_FALSE(set_result.ok);
    EXPECT_NE(set_error.find("parameter ack"), string::npos);
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
        client().Post("/command-router/executions", R"({"command":"query"})", "application/json");
    ASSERT_TRUE(missing_field);
    EXPECT_EQ(missing_field->status, 400);

    auto legacy_fields = client().Post(
        "/command-router/executions",
        json({{"command_type", "query"}, {"command_text", "(Similarity \"human\" %V)"}}).dump(),
        "application/json");
    ASSERT_TRUE(legacy_fields);
    EXPECT_EQ(legacy_fields->status, 400);

    auto bad_command = client().Post(
        "/command-router/executions", make_execution_body("set").dump(), "application/json");
    ASSERT_TRUE(bad_command);
    EXPECT_EQ(bad_command->status, 400);

    auto invalid_command = client().Post(
        "/command-router/executions", make_execution_body("invalid").dump(), "application/json");
    ASSERT_TRUE(invalid_command);
    EXPECT_EQ(invalid_command->status, 400);
}

TEST(CommandExecutionTest, stream_events_use_command_params_envelope) {
    CommandExecution exec(
        "exec-abc",
        "query",
        {{"query", {{"syntax", "metta"}, {"tokens", json::array({"(Similarity %V1 %V2)"})}}}});

    exec.mark_running();
    const json answer = {{"handles", json::array({json::array({"h1"})})}};
    exec.publish_chunk(1, json::array({answer}));
    exec.mark_completed(12, 1);

    size_t next_index = 0;
    bool stream_finished = false;
    auto running = exec.wait_next_event(next_index, chrono::milliseconds(10), stream_finished);
    ASSERT_TRUE(running.has_value());
    auto running_event = json::parse(*running);
    EXPECT_EQ(running_event["command"], CommandExecution::COMMAND_EXECUTION_STATUS);
    EXPECT_EQ(running_event["params"]["status"], "running");
    EXPECT_EQ(running_event["params"]["execution_id"], "exec-abc");

    auto answers = exec.wait_next_event(next_index, chrono::milliseconds(10), stream_finished);
    ASSERT_TRUE(answers.has_value());
    auto answers_event = json::parse(*answers);
    EXPECT_EQ(answers_event["command"], CommandExecution::COMMAND_QUERY_ANSWERS);
    EXPECT_EQ(answers_event["params"]["seq"], 1);
    EXPECT_EQ(answers_event["params"]["received_count"], 1);
    ASSERT_TRUE(answers_event["params"]["answers"].is_array());
    EXPECT_EQ(answers_event["params"]["answers"].size(), 1u);

    auto completed = exec.wait_next_event(next_index, chrono::milliseconds(10), stream_finished);
    ASSERT_TRUE(completed.has_value());
    auto completed_event = json::parse(*completed);
    EXPECT_EQ(completed_event["command"], CommandExecution::COMMAND_EXECUTION_STATUS);
    EXPECT_EQ(completed_event["params"]["status"], "completed");
    EXPECT_EQ(completed_event["params"]["total_items"], 1);
    EXPECT_EQ(completed_event["params"]["duration_ms"], 12);
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

TEST_F(CommandRouterHttpAPIThreadPoolConcurrencyTest, running_executions_bounded_by_thread_pool_size) {
    vector<string> execution_ids;
    for (int i = 0; i < 4; ++i) {
        auto create =
            client().Post("/command-router/executions",
                          make_execution_body("query", SHORT_COMMAND_TEXT + std::to_string(i)).dump(),
                          "application/json");
        ASSERT_TRUE(create);
        ASSERT_EQ(create->status, 202);
        execution_ids.push_back(json::parse(create->body)["execution_id"].get<string>());
    }

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

    EXPECT_GT(max_observed_running, 0);
    EXPECT_LE(max_observed_running, static_cast<int>(kThreadPoolSize));
}

TEST_F(CommandRouterHttpAPIParallelTest, parallel_queries_keep_params_and_answers_isolated) {
    constexpr int N = 20;
    vector<string> errors(N);

    auto worker = [&](int i) {
        const string token = "(Similarity \"c-" + std::to_string(i) + "\" %V)";
        const string expected_key = "(Similarity \"c-" + std::to_string(i) + "\" $V)";
        const unsigned int max_answers = 1u + static_cast<unsigned int>(i % 4);

        json body = {{"command", "query"},
                     {"params",
                      {{"query", {{"syntax", "metta"}, {"tokens", json::array({token})}}},
                       {"use_metta_as_query_tokens", true},
                       {"populate_metta_mapping", false},
                       {"max_answers", max_answers}}}};

        httplib::Client http(TEST_HOST, TEST_PORT_PARALLEL);
        http.set_connection_timeout(2);
        http.set_read_timeout(30);

        auto create = http.Post("/command-router/executions", body.dump(), "application/json");
        if (!create || create->status != 202) {
            errors[i] = "create failed";
            return;
        }
        const string id = json::parse(create->body)["execution_id"].get<string>();

        httplib::ws::WebSocketClient ws("ws://" + TEST_HOST + ":" + std::to_string(TEST_PORT_PARALLEL) +
                                        "/command-router/ws/" + id);
        if (!ws.is_valid() || !ws.connect()) {
            errors[i] = "ws connect failed";
            return;
        }
        ws.set_read_timeout(10, 0);

        vector<string> handles;
        string status;
        string msg;
        while (ws.read(msg)) {
            auto event = json::parse(msg);
            if (event.value("command", "") == CommandExecution::COMMAND_QUERY_ANSWERS) {
                for (const auto& answer : event["params"]["answers"]) {
                    handles.push_back(answer["handles"][0][0].get<string>());
                }
            } else if (event.value("command", "") == CommandExecution::COMMAND_EXECUTION_STATUS) {
                status = event["params"].value("status", "");
            }
        }
        ws.close();

        if (status != "completed") {
            errors[i] = "status=" + status;
            return;
        }
        if (handles.size() != max_answers) {
            errors[i] = "count got=" + std::to_string(handles.size()) +
                        " expected=" + std::to_string(max_answers);
            return;
        }
        for (unsigned int k = 0; k < max_answers; ++k) {
            const string expected = hash_string(expected_key + "#" + std::to_string(k));
            if (handles[k] != expected) {
                errors[i] = "handle mismatch at " + std::to_string(k) + " got=" + handles[k] +
                            " expected=" + expected;
                return;
            }
        }
    };

    vector<std::thread> threads;
    for (int i = 0; i < N; ++i) {
        threads.emplace_back(worker, i);
    }
    for (auto& t : threads) {
        t.join();
    }
    for (int i = 0; i < N; ++i) {
        EXPECT_TRUE(errors[i].empty()) << "client " << i << ": " << errors[i];
    }
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
        ASSERT_TRUE(event.contains("command"));
        ASSERT_TRUE(event.contains("params"));
        EXPECT_EQ(event["params"]["execution_id"].get<string>(), execution_id);
        if (event["command"] == CommandExecution::COMMAND_EXECUTION_STATUS &&
            event["params"].value("status", "") == "running") {
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
    auto api = make_shared<CommandRouterHttpAPI>("localhost", 19002, pool, nullptr);
    CommandRouterHttpAPISingleton::provide(api);
    EXPECT_EQ(CommandRouterHttpAPISingleton::get_instance().get(), api.get());
}

TEST_F(CommandRouterHttpAPISingletonTest, init_after_provide_throws) {
    auto pool = make_shared<ThreadPool>("double_init_pool", 1);
    CommandRouterHttpAPISingleton::provide(
        make_shared<CommandRouterHttpAPI>("localhost", 19005, pool, nullptr));

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
