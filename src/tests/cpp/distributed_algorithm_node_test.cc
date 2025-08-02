#include <cmath>
#include <cstdlib>

#include "DistributedAlgorithmNode.h"
#include "Utils.h"
#include "gtest/gtest.h"

using namespace distributed_algorithm_node;

// -------------------------------------------------------------------------------------------------
// Utility classes - concrete subclasses of DistributedAlgorithmNode and Message

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

class TestNode : public DistributedAlgorithmNode {
   public:
    string server_id;
    bool is_server;
    string command;
    vector<string> args;
    unsigned int node_joined_network_count;

    TestNode(const string& node_id,
             const string& server_id,
             LeadershipBrokerType leadership_algorithm,
             MessageBrokerType messaging_backend,
             bool is_server)
        : DistributedAlgorithmNode(node_id, leadership_algorithm, messaging_backend) {
        this->is_server = is_server;
        if (!is_server) {
            this->server_id = server_id;
            this->add_peer(server_id);
        }
        this->node_joined_network_count = 0;
    }

    virtual ~TestNode() {}

    string cast_leadership_vote() {
        if (this->is_server) {
            return this->node_id();
        } else {
            return this->server_id;
        }
    }

    void node_joined_network(const string& node_id) {
        this->node_joined_network_count += 1;
        if (is_server) {
            this->add_peer(node_id);
        }
    }

    std::shared_ptr<Message> message_factory(string& command, vector<string>& args) {
        std::shared_ptr<Message> message = DistributedAlgorithmNode::message_factory(command, args);
        if (message) {
            return message;
        }
        if (command == "c1" || command == "c2" || command == "c3") {
            return std::shared_ptr<Message>(new TestMessage(command, args));
        }
        return std::shared_ptr<Message>{};
    }
};

void TestMessage::act(shared_ptr<MessageFactory> node) {
    auto distributed_algorithm_node = dynamic_pointer_cast<TestNode>(node);
    distributed_algorithm_node->command = this->command;
    distributed_algorithm_node->args = this->args;
}

// -------------------------------------------------------------------------------------------------
// Test cases

TEST(DistributedAlgorithmNode, basics) {
    string server_id = "localhost:40032";
    string client1_id = "localhost:40033";
    string client2_id = "localhost:40034";
    TestNode* server;
    TestNode* client1;
    TestNode* client2;

    for (auto messaging_type : {MessageBrokerType::RAM, MessageBrokerType::GRPC}) {
        server = new TestNode(
            server_id, server_id, LeadershipBrokerType::SINGLE_MASTER_SERVER, messaging_type, true);
        client1 = new TestNode(
            client1_id, server_id, LeadershipBrokerType::SINGLE_MASTER_SERVER, messaging_type, false);
        client2 = new TestNode(
            client2_id, server_id, LeadershipBrokerType::SINGLE_MASTER_SERVER, messaging_type, false);

        EXPECT_FALSE(server->is_leader());
        EXPECT_FALSE(client1->is_leader());
        EXPECT_FALSE(client2->is_leader());
        EXPECT_FALSE(server->has_leader());
        EXPECT_FALSE(client1->has_leader());
        EXPECT_FALSE(client2->has_leader());
        EXPECT_TRUE(server->leader_id() == "");
        EXPECT_TRUE(client1->leader_id() == "");
        EXPECT_TRUE(client2->leader_id() == "");
        EXPECT_TRUE(server->node_id() == server_id);
        EXPECT_TRUE(client1->node_id() == client1_id);
        EXPECT_TRUE(client2->node_id() == client2_id);

        server->join_network();
        client1->join_network();
        client2->join_network();
        Utils::sleep(1000);

        EXPECT_TRUE(server->is_leader());
        EXPECT_FALSE(client1->is_leader());
        EXPECT_FALSE(client2->is_leader());
        EXPECT_TRUE(server->has_leader());
        EXPECT_TRUE(client1->has_leader());
        EXPECT_TRUE(client2->has_leader());
        EXPECT_TRUE(server->leader_id() == server_id);
        EXPECT_TRUE(client1->leader_id() == server_id);
        EXPECT_TRUE(client2->leader_id() == server_id);
        EXPECT_TRUE(server->node_id() == server_id);
        EXPECT_TRUE(client1->node_id() == client1_id);
        EXPECT_TRUE(client2->node_id() == client2_id);

        delete server;
        delete client1;
        delete client2;
    }
}

TEST(DistributedAlgorithmNode, communication) {
    string server_id = "localhost:40035";
    string client1_id = "localhost:40036";
    string client2_id = "localhost:40037";
    TestNode* server;
    TestNode* client1;
    TestNode* client2;

    for (auto messaging_type : {MessageBrokerType::RAM, MessageBrokerType::GRPC}) {
        server = new TestNode(
            server_id, server_id, LeadershipBrokerType::SINGLE_MASTER_SERVER, messaging_type, true);
        server->join_network();
        client1 = new TestNode(
            client1_id, server_id, LeadershipBrokerType::SINGLE_MASTER_SERVER, messaging_type, false);
        client1->join_network();
        client2 = new TestNode(
            client2_id, server_id, LeadershipBrokerType::SINGLE_MASTER_SERVER, messaging_type, false);
        client2->join_network();
        Utils::sleep(1000);

        EXPECT_EQ(server->command, "");
        EXPECT_EQ(server->args.size(), 0);
        EXPECT_EQ(client1->command, "");
        EXPECT_EQ(client1->args.size(), 0);
        EXPECT_EQ(client2->command, "");
        EXPECT_EQ(client2->args.size(), 0);

        EXPECT_EQ(server->node_joined_network_count, 2);
        EXPECT_EQ(client1->node_joined_network_count, 1);
        EXPECT_EQ(client2->node_joined_network_count, 0);  // TODO Fix this. This count should be 2

        vector<string> args1 = {"a", "b"};
        server->broadcast("c1", args1);
        Utils::sleep(1000);

        EXPECT_EQ(server->command, "");
        EXPECT_EQ(server->args.size(), 0);
        EXPECT_EQ(client1->command, "c1");
        EXPECT_EQ(client1->args, args1);
        EXPECT_EQ(client2->command, "c1");
        EXPECT_EQ(client2->args, args1);

        vector<string> args2 = {"a2", "b2"};
        server->send("c2", args2, client1_id);
        Utils::sleep(1000);

        EXPECT_EQ(server->command, "");
        EXPECT_EQ(server->args.size(), 0);
        EXPECT_EQ(client1->command, "c2");
        EXPECT_EQ(client1->args, args2);
        EXPECT_EQ(client2->command, "c1");
        EXPECT_EQ(client2->args, args1);

        delete server;
        delete client1;
        delete client2;
        Utils::sleep(1000);
    }
}
