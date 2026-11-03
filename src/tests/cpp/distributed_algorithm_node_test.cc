#include <cmath>
#include <cstdlib>
#include <mutex>

#include <grpcpp/grpcpp.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

#include "DistributedAlgorithmNode.h"
#include "Utils.h"
#include "distributed_algorithm_node.grpc.pb.h"
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
    mutex history_mutex;
    vector<pair<string, vector<string>>> history;

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

    void record(const string& command, const vector<string>& args) {
        lock_guard<mutex> lock(this->history_mutex);
        this->command = command;
        this->args = args;
        this->history.emplace_back(command, args);
    }

    vector<pair<string, vector<string>>> snapshot_history() {
        lock_guard<mutex> lock(this->history_mutex);
        return this->history;
    }

    std::shared_ptr<Message> message_factory(string& command, vector<string>& args) {
        std::shared_ptr<Message> message = DistributedAlgorithmNode::message_factory(command, args);
        if (message) {
            return message;
        }
        if (command == "c1" || command == "c2" || command == "c3" || command == "bundle" ||
            command == "finished") {
            return std::shared_ptr<Message>(new TestMessage(command, args));
        }
        return std::shared_ptr<Message>{};
    }
};

void TestMessage::act(shared_ptr<MessageFactory> node) {
    auto distributed_algorithm_node = dynamic_pointer_cast<TestNode>(node);
    distributed_algorithm_node->record(this->command, this->args);
}

/** Serves only unary execute_message; message_stream stays unimplemented. */
class UnaryOnlyService : public dasproto::DistributedAlgorithmNode::Service {
   public:
    mutex history_mutex;
    vector<pair<string, vector<string>>> history;

    grpc::Status ping(grpc::ServerContext*, const dasproto::Empty*, dasproto::Ack* reply) override {
        reply->set_msg("PING");
        return grpc::Status::OK;
    }

    grpc::Status execute_message(grpc::ServerContext*,
                                 const dasproto::MessageData* request,
                                 dasproto::Empty*) override {
        vector<string> args;
        for (int i = 0; i < request->args_size(); i++) {
            args.push_back(request->args(i));
        }
        lock_guard<mutex> lock(this->history_mutex);
        this->history.emplace_back(request->command(), args);
        return grpc::Status::OK;
    }
};

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

TEST(DistributedAlgorithmNode, grpc_stream_preserves_order) {
    string server_id = "localhost:40038";
    string client_id = "localhost:40039";

    auto* server = new TestNode(
        server_id, server_id, LeadershipBrokerType::SINGLE_MASTER_SERVER, MessageBrokerType::GRPC, true);
    server->join_network();
    auto* client = new TestNode(
        client_id, server_id, LeadershipBrokerType::SINGLE_MASTER_SERVER, MessageBrokerType::GRPC, false);
    client->join_network();
    Utils::sleep(500);

    client->send_stream("bundle", {"a1"}, server_id);
    client->send_stream("finished", {}, server_id);

    vector<pair<string, vector<string>>> history;
    for (int i = 0; i < 50; i++) {
        history = server->snapshot_history();
        if (history.size() >= 2) {
            break;
        }
        Utils::sleep(50);
    }

    ASSERT_GE(history.size(), 2u);
    EXPECT_EQ(history[0].first, "bundle");
    EXPECT_EQ(history[0].second, vector<string>({"a1"}));
    EXPECT_EQ(history[1].first, "finished");
    EXPECT_TRUE(history[1].second.empty());

    delete client;
    delete server;
    Utils::sleep(1000);
}

TEST(DistributedAlgorithmNode, grpc_stream_and_unary_coexist) {
    string server_id = "localhost:40040";
    string client_id = "localhost:40041";

    auto* server = new TestNode(
        server_id, server_id, LeadershipBrokerType::SINGLE_MASTER_SERVER, MessageBrokerType::GRPC, true);
    server->join_network();
    auto* client = new TestNode(
        client_id, server_id, LeadershipBrokerType::SINGLE_MASTER_SERVER, MessageBrokerType::GRPC, false);
    client->join_network();
    Utils::sleep(500);

    client->send_stream("bundle", {"s1"}, server_id);
    client->send("c2", {"u1"}, server_id);
    client->send_stream("finished", {"s2"}, server_id);

    vector<pair<string, vector<string>>> history;
    for (int i = 0; i < 50; i++) {
        history = server->snapshot_history();
        if (history.size() >= 3) {
            break;
        }
        Utils::sleep(50);
    }

    ASSERT_GE(history.size(), 3u);
    bool saw_bundle = false;
    bool saw_finished = false;
    bool saw_unary = false;
    for (const auto& entry : history) {
        if (entry.first == "bundle") {
            saw_bundle = true;
            EXPECT_FALSE(saw_finished);
        } else if (entry.first == "finished") {
            saw_finished = true;
            EXPECT_TRUE(saw_bundle);
        } else if (entry.first == "c2") {
            saw_unary = true;
        }
    }
    EXPECT_TRUE(saw_bundle);
    EXPECT_TRUE(saw_finished);
    EXPECT_TRUE(saw_unary);

    delete client;
    delete server;
    Utils::sleep(1000);
}

TEST(DistributedAlgorithmNode, grpc_stream_falls_back_to_unary) {
    string unary_server_id = "localhost:40042";
    string client_id = "localhost:40043";

    UnaryOnlyService unary_service;
    grpc::ServerBuilder builder;
    builder.AddListeningPort(unary_server_id, grpc::InsecureServerCredentials());
    builder.RegisterService(&unary_service);
    unique_ptr<grpc::Server> unary_server = builder.BuildAndStart();
    ASSERT_NE(unary_server, nullptr);

    auto* client = new TestNode(
        client_id, client_id, LeadershipBrokerType::SINGLE_MASTER_SERVER, MessageBrokerType::GRPC, true);
    client->join_network();
    client->add_peer(unary_server_id);
    Utils::sleep(300);

    client->send_stream("c3", {"fallback"}, unary_server_id);

    bool got_message = false;
    for (int i = 0; i < 50; i++) {
        lock_guard<mutex> lock(unary_service.history_mutex);
        if (!unary_service.history.empty()) {
            EXPECT_EQ(unary_service.history[0].first, "c3");
            EXPECT_EQ(unary_service.history[0].second, vector<string>({"fallback"}));
            got_message = true;
            break;
        }
        Utils::sleep(50);
    }
    EXPECT_TRUE(got_message);

    delete client;
    unary_server->Shutdown();
    unary_server->Wait();
    Utils::sleep(300);
}
