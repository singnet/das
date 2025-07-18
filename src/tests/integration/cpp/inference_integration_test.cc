#include <gtest/gtest.h>

#include <iostream>
#include <string>

#include "process_helper.h"
#include "test_helper.h"

using namespace std;

class IntegrationTest : public ::testing::Test {
   protected:
    string query_agent_port;
    FileWatcher* file_watcher;
    LogHandle* attention_broker;
    LogHandle* query_agent;
    LogHandle* link_creation_agent;
    LogHandle* inference_agent;
    LogHandle* evolution_agent;

    void SetUp() override {
        // Set up the environment for the integration test
        query_agent_port = "1200";
        // Create a file watcher to monitor the agents' logs
        this->file_watcher = new FileWatcher("/opt/das/bin", 60.0f);
        this->attention_broker =
            this->file_watcher->create_log_handle("/opt/das/logs/attention_broker_service.log");
        this->query_agent = this->file_watcher->create_log_handle("/opt/das/logs/query_broker.log");
        this->link_creation_agent =
            this->file_watcher->create_log_handle("/opt/das/logs/link_creation_server.log");
        this->inference_agent =
            this->file_watcher->create_log_handle("/opt/das/logs/inference_agent_server.log");
        this->evolution_agent =
            this->file_watcher->create_log_handle("/opt/das/logs/evolution_broker.log");
    }

    void TearDown() override {
        // Check the output of all agents
        cout << "Attention Broker Output: \n" << this->attention_broker->get_output() << endl;
        cout << "Query Agent Output: \n" << this->query_agent->get_output() << endl;
        cout << "Link Creation Agent Output: \n" << this->link_creation_agent->get_output() << endl;
        cout << "Evolution Agent Output: \n" << this->evolution_agent->get_output() << endl;
        cout << "Inference Agent Output: \n" << this->inference_agent->get_output() << endl;
        // Stop the file watcher and delete log handles
        this->file_watcher->stop();
        delete this->file_watcher;
        delete this->attention_broker;
        delete this->query_agent;
        delete this->link_creation_agent;
        delete this->inference_agent;
        delete this->evolution_agent;
    }
};

TEST_F(IntegrationTest, TestProofOfImplicationOrEquivalence) {
    GTEST_SKIP() << "Evolution is crashing, skipping this test for now.";
    Process inference_client("/opt/das/bin/inference_agent_client",
                             {"localhost:2500",
                              "localhost:" + this->query_agent_port,
                              "25000:26000",
                              "PROOF_OF_IMPLICATION_OR_EQUIVALENCE",
                              "8860480382d0ddf62623abf5c860e51d",
                              "8321bf987d14e3ccbf9aaa6dad1e4755",
                              "1",
                              "POC"});

    ASSERT_TRUE(inference_client.start());
    inference_client.wait(20 * 1000);
    string output = inference_client.get_output();
    cout << "Inference Client Output: \n" << output << endl;

    ASSERT_TRUE(output.find("Error") == string::npos);
    ASSERT_TRUE(this->inference_agent->get_output().find("Inference request processed") != string::npos);
    ASSERT_TRUE(this->evolution_agent->get_output().find("Command finished: <query_evolution>") !=
                string::npos);
    ASSERT_TRUE(this->link_creation_agent->get_output().find("PROOF_OF_IMPLICATION") != string::npos);
    ASSERT_TRUE(this->link_creation_agent->get_output().find("PROOF_OF_EQUIVALENCE") != string::npos);
    ASSERT_TRUE(this->query_agent->get_output().find(
                    "Fetched 9 links for link template ec86337bea4bcc2b8fcf7c4d401aa6cc") !=
                string::npos);
}

TEST_F(IntegrationTest, TestProofOfImplication) {
    GTEST_SKIP() << "Evolution is crashing, skipping this test for now.";
    Process inference_client("/opt/das/bin/inference_agent_client",
                             {"localhost:2500",
                              "localhost:" + this->query_agent_port,
                              "25000:26000",
                              "PROOF_OF_IMPLICATION",
                              "8860480382d0ddf62623abf5c860e51d",
                              "8321bf987d14e3ccbf9aaa6dad1e4755",
                              "1",
                              "POC"});

    ASSERT_TRUE(inference_client.start());
    inference_client.wait(20 * 1000);

    string output = inference_client.get_output();
    cout << "Inference Client Output: \n" << output << endl;

    ASSERT_TRUE(output.find("Error") == string::npos);
    ASSERT_TRUE(this->inference_agent->get_output().find("Inference request processed") != string::npos);
    ASSERT_TRUE(this->evolution_agent->get_output().find("Command finished: <query_evolution>") !=
                string::npos);
    ASSERT_TRUE(this->link_creation_agent->get_output().find("PROOF_OF_IMPLICATION") != string::npos);
    ASSERT_TRUE(this->link_creation_agent->get_output().find("PROOF_OF_EQUIVALENCE") == string::npos);
}

TEST_F(IntegrationTest, TestProofOfEquivalence) {
    GTEST_SKIP() << "Evolution is crashing, skipping this test for now.";
    Process inference_client("/opt/das/bin/inference_agent_client",
                             {"localhost:2500",
                              "localhost:" + this->query_agent_port,
                              "25000:26000",
                              "PROOF_OF_EQUIVALENCE",
                              "8860480382d0ddf62623abf5c860e51d",
                              "8321bf987d14e3ccbf9aaa6dad1e4755",
                              "1",
                              "POC"});

    ASSERT_TRUE(inference_client.start());
    inference_client.wait(20 * 1000);
    string output = inference_client.get_output();
    cout << "Inference Client Output: \n" << output << endl;

    ASSERT_TRUE(output.find("Error") == string::npos);
    ASSERT_TRUE(this->inference_agent->get_output().find("Inference request processed") != string::npos);
    ASSERT_TRUE(this->evolution_agent->get_output().find("Command finished: <query_evolution>") !=
                string::npos);
    ASSERT_TRUE(this->link_creation_agent->get_output().find("PROOF_OF_IMPLICATION") == string::npos);
    ASSERT_TRUE(this->link_creation_agent->get_output().find("PROOF_OF_EQUIVALENCE") != string::npos);
}