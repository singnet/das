#include <cstdlib>

#include "ServiceBusSingleton.h"
#include "Utils.h"
#include "gtest/gtest.h"

using namespace service_bus;

class TestProxy : public BusCommandProxy {
   public:
    string remote_command;
    vector<string> remote_args;
    TestProxy() {}
    TestProxy(const string& command, vector<string>& args) : BusCommandProxy(command, args) {}
    bool from_remote_peer(const string& command, const vector<string>& args) {
        this->remote_command = command;
        this->remote_args = args;
        return true;
    }
    void pack_command_line_args() {}
};

class TestProcessor : public BusCommandProcessor {
   public:
    string command;
    vector<string> args;
    shared_ptr<BusCommandProxy> proxy;

    TestProcessor(const set<string>& commands) : BusCommandProcessor(commands) {}

    shared_ptr<BusCommandProxy> factory_empty_proxy() {
        shared_ptr<BusCommandProxy> p(new TestProxy());
        return p;
    }

    void run_command(shared_ptr<BusCommandProxy> proxy) {
        this->command = proxy->get_command();
        this->args = proxy->get_args();
        this->proxy = proxy;
    }
};

void check_command(ServiceBus& source, shared_ptr<TestProcessor> target, const string& command) {
    vector<string> args;
    string arg = command + "_arg";
    args.push_back(arg);
    shared_ptr<TestProxy> proxy(new TestProxy(command, args));
    source.issue_bus_command(proxy);
    Utils::sleep(1000);
    EXPECT_EQ(target->command, command);
    EXPECT_EQ(target->args[0], arg);

    string ping = command + "_ping";
    string ping_arg = ping + "_arg";
    proxy->to_remote_peer(ping, {ping_arg});
    Utils::sleep(1000);
    EXPECT_EQ(dynamic_pointer_cast<TestProxy>(target->proxy)->remote_command, ping);
    EXPECT_EQ(dynamic_pointer_cast<TestProxy>(target->proxy)->remote_args[0], ping_arg);
}

TEST(ServiceBus, basics) {
    set<string> commands = {"c1", "c2", "c3", "c4", "c5"};
    ServiceBus::initialize_statics(commands, 72400, 73400);
    shared_ptr<TestProcessor> processor1(new TestProcessor({"c1", "c4"}));
    shared_ptr<TestProcessor> processor2(new TestProcessor({"c2"}));
    shared_ptr<TestProcessor> processor3(new TestProcessor({"c3"}));
    string peer1_id = "localhost:32701";
    string peer2_id = "localhost:32702";
    string peer3_id = "localhost:32703";

    ServiceBus service_bus1(peer1_id);
    Utils::sleep(1000);
    ServiceBus service_bus2(peer2_id, peer1_id);
    ServiceBus service_bus3(peer3_id, peer1_id);
    Utils::sleep(1000);
    service_bus1.register_processor(processor1);
    service_bus2.register_processor(processor2);
    service_bus3.register_processor(processor3);
    Utils::sleep(1000);

    check_command(service_bus2, processor1, "c1");
    check_command(service_bus3, processor1, "c1");
    check_command(service_bus1, processor2, "c2");
    check_command(service_bus3, processor2, "c2");
    check_command(service_bus1, processor3, "c3");
    check_command(service_bus2, processor3, "c3");
    check_command(service_bus2, processor1, "c4");
    check_command(service_bus3, processor1, "c4");
}
