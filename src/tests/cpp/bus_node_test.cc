#include <cstdlib>
#include "gtest/gtest.h"

#include "BusNode.h"
#include "Utils.h"

using namespace distributed_algorithm_node;


class TestMessage : public Message {
public:
    string command;
    vector<string> args;
    TestMessage(string command, vector<string> args) {
        this->command = command;
        this->args = args;
    }
    void act(shared_ptr<MessageFactory> node);
};

class TestNode : public BusNode {

public:

    string id;
    BusNode::Bus bus;
    string command;
    vector<string> args;

    TestNode(
        const string &id, 
        const BusNode::Bus &bus, 
        const set<string> &node_commands,
        const string &known_peer) : BusNode (
            id,
            bus,
            node_commands,
            known_peer,
            MessageBrokerType::RAM
        ) {

        this->id = id;
        this->bus = bus;
    }

    virtual ~TestNode() {
    }

    std::shared_ptr<Message> message_factory(string &command, vector<string> &args) {
        set<string> commands = {"c1", "c2", "c3", "c4", "c5", "c6"};
        std::shared_ptr<Message> message = BusNode::message_factory(command, args);
        if (message) {
            return message;
        }
        if (commands.find(command) != commands.end()) {
            return std::shared_ptr<Message>(new TestMessage(command, args));
        }
        return std::shared_ptr<Message>{};
    }
};

void TestMessage::act(shared_ptr<MessageFactory> node) {
    auto test_node = dynamic_pointer_cast<TestNode>(node);
    test_node->command = this->command;
    test_node->args = this->args;
}

static void check(TestNode &sender, const string &command, TestNode &receiver) {
    sender.send_bus_command(command, {command + "arg1"});
    Utils::sleep(1000);
    EXPECT_EQ(receiver.command, command);
    EXPECT_EQ(receiver.args[0], command + "arg1");
}

TEST(BusNode, send) {

    BusNode::Bus bus;
    bus.add("c1");
    bus.add("c2");
    bus.add("c3");
    bus.add("c4");
    bus.add("c5");

    TestNode node1("bus_node1", bus, {"c1"}, "");
    TestNode node2("bus_node2", bus, {"c2", "c4"}, "bus_node1");
    TestNode node3("bus_node3", bus, {"c3"}, "bus_node2");
    Utils::sleep(5000);

    check(node1, "c2", node2);
    check(node1, "c3", node3);
    check(node2, "c1", node1);
    check(node2, "c3", node3);
    check(node3, "c1", node1);
    check(node3, "c2", node2);
    check(node1, "c4", node2);
    check(node3, "c4", node2);

    try {
        check(node1, "c5", node2);
        FAIL() << "Expected exception";
    } catch(std::runtime_error const &error) {
    } catch(...) {
        FAIL() << "Expected std::runtime_error";
    }

    try {
        check(node1, "c6", node2);
        FAIL() << "Expected exception";
    } catch(std::runtime_error const &error) {
    } catch(...) {
        FAIL() << "Expected std::runtime_error";
    }

    Utils::sleep(1000);
}

TEST(BusNode, basics) {

    string node1_id = "node1";
    string node2_id = "node2";
    string node3_id = "node3";

    BusNode::Bus bus;
    bus.add("c1");
    bus.add("c2");
    bus.add("c3");
    bus.add("c4");

    std::set<string> node1_commands = {"c1", "c2"};
    std::set<string> node2_commands = {"c3"};
    std::set<string> node3_commands = {};

    BusNode node1(node1_id, bus, node1_commands, "", MessageBrokerType::RAM);

    EXPECT_TRUE(node1.is_leader());
    EXPECT_TRUE(node1.has_leader());
    EXPECT_EQ(node1.leader_id(), node1_id);
    EXPECT_EQ(node1.node_id(), node1_id);

    BusNode node2(node2_id, bus, node2_commands, node1_id, MessageBrokerType::RAM);
    BusNode node3(node3_id, bus, node3_commands, node2_id, MessageBrokerType::RAM);

    EXPECT_TRUE(node1.is_leader());
    EXPECT_TRUE(node1.has_leader());
    EXPECT_EQ(node1.leader_id(), node1_id);
    EXPECT_EQ(node1.node_id(), node1_id);

    EXPECT_FALSE(node2.is_leader());
    EXPECT_TRUE(node2.has_leader());
    EXPECT_EQ(node2.leader_id(), node1_id);
    EXPECT_EQ(node2.node_id(), node2_id);

    EXPECT_FALSE(node3.is_leader());
    EXPECT_TRUE(node3.has_leader());
    EXPECT_EQ(node3.leader_id(), node2_id);
    EXPECT_EQ(node3.node_id(), node3_id);

    Utils::sleep(1000);

    for (auto node: {node1, node2, node3}) {
        EXPECT_EQ(node.get_ownership("c1"), node1_id);
        EXPECT_EQ(node.get_ownership("c2"), node1_id);
        EXPECT_EQ(node.get_ownership("c3"), node2_id);
        EXPECT_EQ(node.get_ownership("c4"), "");
    }
}

TEST(BusNode, bus) {

    BusNode::Bus bus1;
    bus1.add("c1");
    bus1.add("c2");
    bus1.add("c3");

    BusNode::Bus bus2(bus1);
    BusNode::Bus bus3 = bus2;

    BusNode::Bus bus4;
    bus4 = bus4 + "c1";
    bus4 = bus4 + "c2";
    bus4 = bus4 + "c3";
    BusNode::Bus bus5;
    bus5 = ((bus5 + "c1") + "c2") + "c3";

    EXPECT_TRUE(bus1 == bus2);
    EXPECT_TRUE(bus2 == bus3);
    EXPECT_TRUE(bus3 == bus4);
    EXPECT_TRUE(bus4 == bus5);
    EXPECT_TRUE(bus5 == bus1);

    bus1.set_ownership("c1", "a");
    bus1.set_ownership("c2", "a");
    bus1.set_ownership("c1", "a");
    try {
        bus1.set_ownership("c1", "b");
        FAIL() << "Expected exception";
    } catch(std::runtime_error const &error) {
    } catch(...) {
        FAIL() << "Expected std::runtime_error";
    }
}
