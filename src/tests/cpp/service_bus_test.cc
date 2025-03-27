#include <cstdlib>

#include "ServiceBusSingleton.h"
#include "Utils.h"
#include "gtest/gtest.h"

using namespace service_bus;

class TestProcessor : public BusCommandProcessor {
   public:
    string command;
    vector<string> args;

    TestProcessor(const set<string>& commands) : BusCommandProcessor(commands) {}

    void run_command(const string& command, const vector<string>& args) {
        this->command = command;
        this->args = args;
    }
};

TEST(ServiceBus, basics) {
    set<string> commands = {"c1", "c2", "c3", "c4", "c5"};
    shared_ptr<TestProcessor> processor1(new TestProcessor({"c1", "c4"}));
    shared_ptr<TestProcessor> processor2(new TestProcessor({"c2"}));
    shared_ptr<TestProcessor> processor3(new TestProcessor({"c3"}));
    string peer1_id = "localhost:32700";
    string peer2_id = "localhost:32701";
    string peer3_id = "localhost:32702";
    BusNode::Bus bus;
    for (auto command : commands) {
        bus.add(command);
    }
    ServiceBus::initialize_statics(commands);

    ServiceBus service_bus1(peer1_id);
    Utils::sleep(1000);
    ServiceBus service_bus2(peer2_id);
    ServiceBus service_bus3(peer3_id);
    Utils::sleep(1000);
    service_bus1.register_processor(processor1);
    service_bus2.register_processor(processor2);
    service_bus3.register_processor(processor3);
    Utils::sleep(1000);
}
