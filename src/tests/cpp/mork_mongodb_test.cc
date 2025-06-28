#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <cstring>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "AtomDBSingleton.h"
#include "LinkTemplateInterface.h"
#include "MorkMongoDB.h"

using namespace atomdb;
using namespace atomspace;

class MorkMongoDBTestEnvironment : public ::testing::Environment {
   public:
    void SetUp() override {
        setenv("DAS_REDIS_HOSTNAME", "localhost", 1);
        setenv("DAS_REDIS_PORT", "29000", 1);
        setenv("DAS_USE_REDIS_CLUSTER", "false", 1);
        setenv("DAS_MONGODB_HOSTNAME", "localhost", 1);
        setenv("DAS_MONGODB_PORT", "28000", 1);
        setenv("DAS_MONGODB_USERNAME", "dbadmin", 1);
        setenv("DAS_MONGODB_PASSWORD", "dassecret", 1);
        setenv("DAS_DISABLE_ATOMDB_CACHE", "true", 1);
        setenv("DAS_MORK_HOSTNAME", "localhost", 1);
        setenv("DAS_MORK_PORT", "8000", 1);
        AtomDBSingleton::init(atomdb_api_types::ATOMDB_TYPE::MORK_MONGODB);
    }
};

class MockMorkClient : public MorkClient {
   public:
    MockMorkClient(const string& base_url = "localhost:8000") : MorkClient(base_url) {}
    MOCK_METHOD(string, post, (const string& data, const string& pattern, const string& template_));
    MOCK_METHOD(vector<string>, get, (const string& pattern, const string& template_));
};

class MorkMongoDBTest : public ::testing::Test {
   protected:
    void SetUp() override {
        auto atomdb = AtomDBSingleton::get_instance();
        db = dynamic_pointer_cast<MorkMongoDB>(atomdb);
        db->set_mork_client_for_test(mock_client);
        ASSERT_NE(db, nullptr) << "Failed to cast AtomDB to MorkMongoDB";
    }

    void TearDown() override {}

    shared_ptr<char> handle_ptr(string str) {
        size_t len = str.length() + 1;
        auto buffer = make_unique<char[]>(len);
        strcpy(buffer.get(), str.c_str());
        return shared_ptr<char>(buffer.release(), default_delete<char[]>());
    }

    string exp_hash(vector<string> targets) {
        char* symbol = (char*) "Symbol";
        char** targets_handles = new char*[targets.size()];
        for (size_t i = 0; i < targets.size(); i++) {
            targets_handles[i] = terminal_hash(symbol, (char*) targets[i].c_str());
        }
        char* expression = named_type_hash((char*) "Expression");
        return string(expression_hash(expression, targets_handles, targets.size()));
    }

    shared_ptr<MockMorkClient> mock_client = make_shared<MockMorkClient>();
    shared_ptr<MorkMongoDB> db;
};

class LinkTemplateMetta : public LinkTemplateInterface {
   public:
    LinkTemplateMetta(const string metta_expr) : metta_expr(metta_expr) {}
    const char* get_handle() const override { return ""; }
    string get_metta_expression() const override { return metta_expr; }

   private:
    const string metta_expr;
};

TEST_F(MorkMongoDBTest, QueryForPattern) {
    auto node_evaluation = new atomspace::Node("Symbol", "EVALUATION");
    auto node_predicate = new atomspace::Node("Symbol", "PREDICATE");
    auto node_concept = new atomspace::Node("Symbol", "CONCEPT");
    auto node_a = new atomspace::Node("Symbol", "A");
    auto node_b = new atomspace::Node("Symbol", "B");
    auto node_c = new atomspace::Node("Symbol", "C");

    auto link_predicate = new atomspace::Link("Expression", {node_predicate, node_a});
    auto link_concept1 = new atomspace::Link("Expression", {node_concept, node_b});
    auto link_concept2 = new atomspace::Link("Expression", {node_concept, node_c});

    auto link_evaluation1 =
        new atomspace::Link("Expression", {node_evaluation, link_predicate, link_concept1});
    auto link_evaluation2 =
        new atomspace::Link("Expression", {node_evaluation, link_predicate, link_concept2});

    string metta = R"(EVALUATION (PREDICATE A) $x)";
    vector<string> fake_exprs = {R"((EVALUATION  (PREDICATE A)  (CONCEPT B)))",
                                 R"((EVALUATION  (PREDICATE A)  (CONCEPT C)))"};
    EXPECT_CALL(*mock_client, get(metta, metta)).Times(1).WillOnce(::testing::Return(fake_exprs));

    auto link_template = new LinkTemplateMetta(metta);
    auto result = db->query_for_pattern(*link_template);

    ASSERT_EQ(result->size(), 2);

    auto it = result->get_iterator();
    char* handle;
    vector<string> handles;
    while ((handle = it->next()) != nullptr) {
        handles.push_back(handle);
    }

    auto h1 = atomspace::Link::compute_handle(link_evaluation1->type, link_evaluation1->targets);
    auto h2 = atomspace::Link::compute_handle(link_evaluation2->type, link_evaluation2->targets);

    ASSERT_EQ(h1, h2);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new MorkMongoDBTestEnvironment());
    return RUN_ALL_TESTS();
}
