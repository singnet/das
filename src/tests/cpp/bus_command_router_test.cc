#include "AtomDBSingleton.h"
#include "BusCommandRouterProcessor.h"
#include "BusCommandRouterProxy.h"
#include "CommandLineParser.h"
#include "ServiceBus.h"
#include "TestAtomDBJsonConfig.h"
#include "Utils.h"
#include "gtest/gtest.h"

using namespace command_router;
using namespace service_bus;
using namespace atomdb;

class BusCommandRouterTestEnvironment : public ::testing::Environment {
   public:
    void SetUp() override { AtomDBSingleton::init(test_atomdb_json_config()); }
    void TearDown() override {}
};

class CaptureForwardProxy : public BusCommandProxy {
   public:
    void pack_command_line_args() override {}
};

class CaptureForwardProcessor : public BusCommandProcessor {
   public:
    string last_requestor_id;
    string last_proxy_node_id;
    vector<string> last_packed_args;

    CaptureForwardProcessor() : BusCommandProcessor({ServiceBus::PATTERN_MATCHING_QUERY}) {}

    shared_ptr<BusCommandProxy> factory_empty_proxy() override {
        return make_shared<CaptureForwardProxy>();
    }

    void run_command(shared_ptr<BusCommandProxy> proxy) override {
        last_requestor_id = proxy->get_requestor_id();
        last_proxy_node_id = proxy->peer_id();
        last_packed_args = proxy->get_args();
    }
};

class RouterTestProxy : public BusCommandRouterProxy {
   public:
    RouterTestProxy(const string& router_command, const string& router_arg)
        : BusCommandRouterProxy(router_command, router_arg) {}

    bool from_remote_peer(const string& command, const vector<string>& args) override {
        if (command == BusCommandRouterProxy::PARAMS_RESPONSE && !args.empty()) {
            params_response = args[0];
            return true;
        }
        if (command == BusCommandRouterProxy::SET_PARAM_ACK && !args.empty()) {
            set_param_ack = args[0];
            return true;
        }
        if (command == BusCommandRouterProxy::ROUTED) {
            routed_flag = true;
            return true;
        }
        if (BusCommandRouterProxy::from_remote_peer(command, args)) {
            return true;
        }
        if (command == BusCommandRouterProxy::PARAMS_RESPONSE && !args.empty()) {
            params_response = args[0];
            return true;
        }
        if (command == BusCommandRouterProxy::SET_PARAM_ACK && !args.empty()) {
            set_param_ack = args[0];
            return true;
        }
        if (command == BusCommandRouterProxy::ROUTED) {
            routed_flag = true;
            return true;
        }
        return false;
    }
};

TEST(CommandLineParser, split_command_line) {
    auto parts = split_command_line("query (Similarity $a $b)");
    EXPECT_EQ(parts.first, "query");
    EXPECT_EQ(parts.second, "(Similarity $a $b)");

    auto parts2 = split_command_line("set param max_answers 777");
    EXPECT_EQ(parts2.first, "set");
    EXPECT_EQ(parts2.second, "param max_answers 777");
}

TEST(BusCommandRouter, get_and_set_params) {
    set<string> commands = {ServiceBus::BUS_COMMAND_ROUTER};
    ServiceBus::initialize_statics(commands, 40500, 40599);

    auto router_processor = make_shared<BusCommandRouterProcessor>();
    ServiceBus router_bus("localhost:40510");
    router_bus.register_processor(router_processor);
    Utils::sleep(500);

    ServiceBus client_bus("localhost:40511", "localhost:40510");
    Utils::sleep(500);

    auto get_proxy = make_shared<RouterTestProxy>("get", "params");
    client_bus.issue_bus_command(get_proxy);
    Utils::sleep(1000);
    EXPECT_FALSE(get_proxy->params_response.empty());
    EXPECT_TRUE(get_proxy->finished());

    auto set_proxy = make_shared<RouterTestProxy>("set", "param max_answers 777");
    client_bus.issue_bus_command(set_proxy);
    Utils::sleep(1000);
    EXPECT_EQ(set_proxy->set_param_ack, "ok max_answers 777");
    EXPECT_TRUE(set_proxy->finished());

    auto get_after_set = make_shared<RouterTestProxy>("get", "params");
    client_bus.issue_bus_command(get_after_set);
    Utils::sleep(1000);
    EXPECT_NE(get_after_set->params_response.find("max_answers\n777"), string::npos);
    EXPECT_TRUE(get_after_set->finished());

    auto set_one = make_shared<RouterTestProxy>("set", "param max_answers 1");
    client_bus.issue_bus_command(set_one);
    Utils::sleep(1000);
    EXPECT_EQ(set_one->set_param_ack, "ok max_answers 1");

    auto get_one = make_shared<RouterTestProxy>("get", "params");
    client_bus.issue_bus_command(get_one);
    Utils::sleep(1000);
    EXPECT_NE(get_one->params_response.find("max_answers\n1"), string::npos);
    EXPECT_EQ(get_one->params_response.find("max_answers\ntrue"), string::npos);
}

TEST(BusCommandRouter, params_isolated_per_peer) {
    set<string> commands = {ServiceBus::BUS_COMMAND_ROUTER};
    ServiceBus::initialize_statics(commands, 40700, 40799);

    auto router_processor = make_shared<BusCommandRouterProcessor>();
    ServiceBus router_bus("localhost:40710");
    router_bus.register_processor(router_processor);
    Utils::sleep(500);

    ServiceBus client_a("localhost:40711", "localhost:40710");
    ServiceBus client_b("localhost:40712", "localhost:40710");
    Utils::sleep(500);

    auto set_a = make_shared<RouterTestProxy>("set", "param max_answers 111");
    client_a.issue_bus_command(set_a);
    Utils::sleep(1000);

    auto set_b = make_shared<RouterTestProxy>("set", "param max_answers 222");
    client_b.issue_bus_command(set_b);
    Utils::sleep(1000);

    auto get_a = make_shared<RouterTestProxy>("get", "params");
    client_a.issue_bus_command(get_a);
    Utils::sleep(1000);
    EXPECT_NE(get_a->params_response.find("max_answers\n111"), string::npos);
    EXPECT_EQ(get_a->params_response.find("max_answers\n222"), string::npos);

    auto get_b = make_shared<RouterTestProxy>("get", "params");
    client_b.issue_bus_command(get_b);
    Utils::sleep(1000);
    EXPECT_NE(get_b->params_response.find("max_answers\n222"), string::npos);
    EXPECT_EQ(get_b->params_response.find("max_answers\n111"), string::npos);
}

TEST(BusCommandRouter, forward_query_preserves_caller_identity) {
    set<string> commands = {ServiceBus::BUS_COMMAND_ROUTER, ServiceBus::PATTERN_MATCHING_QUERY};
    ServiceBus::initialize_statics(commands, 40600, 40699);

    auto query_processor = make_shared<CaptureForwardProcessor>();
    shared_ptr<ServiceBus> query_bus = make_shared<ServiceBus>("localhost:40610");
    query_bus->register_processor(query_processor);
    Utils::sleep(500);

    shared_ptr<ServiceBus> router_bus = make_shared<ServiceBus>("localhost:40611", "localhost:40610");
    auto router_processor = make_shared<BusCommandRouterProcessor>(router_bus);
    router_bus->register_processor(router_processor);
    Utils::sleep(1000);

    ServiceBus client_bus("localhost:40612", "localhost:40611");
    Utils::sleep(500);

    auto query_proxy = make_shared<RouterTestProxy>("query", "(Similarity $a $b)");
    client_bus.issue_bus_command(query_proxy);
    Utils::sleep(1500);

    EXPECT_TRUE(query_proxy->routed_flag);
    // Router issues the downstream query (requestor is the router bus node).
    EXPECT_EQ(query_processor->last_requestor_id, "localhost:40611");
    // Answers return to the router's query proxy port, not the client's listen address.
    EXPECT_NE(query_processor->last_proxy_node_id, query_proxy->my_id());
    bool found_query = false;
    for (const auto& arg : query_processor->last_packed_args) {
        if (arg.find("Similarity") != string::npos) {
            found_query = true;
            break;
        }
    }
    EXPECT_TRUE(found_query);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new BusCommandRouterTestEnvironment());
    return RUN_ALL_TESTS();
}
