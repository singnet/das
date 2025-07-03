#include "MorkDB.h"

#include <gtest/gtest.h>

#include <atomic>
#include <cstring>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "AtomDBSingleton.h"
#include "LinkTemplateInterface.h"

using namespace atomdb;
using namespace atomspace;
using namespace std;

class MorkDBTestEnvironment : public ::testing::Environment {
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
        AtomDBSingleton::init(atomdb_api_types::ATOMDB_TYPE::MORKDB);
    }
};

class MorkDBTest : public ::testing::Test {
   protected:
    void SetUp() override {
        auto atomdb = AtomDBSingleton::get_instance();
        db = dynamic_pointer_cast<MorkDB>(atomdb);
        ASSERT_NE(db, nullptr) << "Failed to cast AtomDB to MorkDB";
    }

    void TearDown() override {}

    string exp_hash(vector<string> targets) {
        char* symbol = (char*) "Symbol";
        char** targets_handles = new char*[targets.size()];
        for (size_t i = 0; i < targets.size(); i++) {
            targets_handles[i] = terminal_hash(symbol, (char*) targets[i].c_str());
        }
        char* expression = named_type_hash((char*) "Expression");
        return string(expression_hash(expression, targets_handles, targets.size()));
    }

    shared_ptr<MorkDB> db;
};

class LinkTemplateMetta : public LinkTemplateInterface {
   public:
    LinkTemplateMetta(const string metta_expr) : metta_expr(metta_expr) {}
    const char* get_handle() const override { return ""; }
    string get_metta_expression() const override { return metta_expr; }

   private:
    const string metta_expr;
};

TEST_F(MorkDBTest, QueryForPattern) {
    string handle_1 = exp_hash({"Inheritance", "\"human\"", "\"mammal\""});
    string handle_2 = exp_hash({"Inheritance", "\"monkey\"", "\"mammal\""});
    string handle_3 = exp_hash({"Inheritance", "\"chimp\"", "\"mammal\""});
    string handle_4 = exp_hash({"Inheritance", "\"rhino\"", "\"mammal\""});

    string metta = R"((Inheritance $x "mammal"))";

    auto link_template = new LinkTemplateMetta(metta);
    auto result = db->query_for_pattern(*link_template);
    delete link_template;

    ASSERT_EQ(result->size(), 4);

    auto it = result->get_iterator();
    char* handle;
    vector<string> handles;
    while ((handle = it->next()) != nullptr) {
        handles.push_back(handle);
    }

    vector<string> expected_handles = {handle_1, handle_2, handle_3, handle_4};

    std::sort(handles.begin(), handles.end());
    std::sort(expected_handles.begin(), expected_handles.end());

    ASSERT_EQ(handles, expected_handles);
}

TEST_F(MorkDBTest, QueryForTargets) {
    string link_handle = exp_hash({"Inheritance", "\"human\"", "\"mammal\""});

    auto targets_list = db->query_for_targets(link_handle);
    ASSERT_NE(targets_list, nullptr);

    unsigned int count = targets_list->size();
    ASSERT_EQ(count, 3);

    vector<string> targets;
    for (unsigned int i = 0; i < count; ++i) {
        const char* raw = targets_list->get_handle(i);
        ASSERT_NE(raw, nullptr);
        targets.push_back(raw);
    }

    string inheritance_handle = terminal_hash((char*) "Symbol", (char*) "Inheritance");
    string human_handle = terminal_hash((char*) "Symbol", (char*) "\"human\"");
    string mammal_handle = terminal_hash((char*) "Symbol", (char*) "\"mammal\"");

    vector<string> expected = {inheritance_handle, human_handle, mammal_handle};
    std::sort(targets.begin(), targets.end());
    std::sort(expected.begin(), expected.end());

    ASSERT_EQ(targets, expected);
}

TEST_F(MorkDBTest, ConcurrentQueryForPattern) {
    const int num_threads = 200;
    vector<thread> threads;
    atomic<int> success_count{0};

    auto worker = [&](int thread_id) {
        try {
            string metta = R"((Similarity $x $y))";

            auto link_template = new LinkTemplateMetta(metta);
            auto handle_set = db->query_for_pattern(*link_template);
            ASSERT_NE(handle_set, nullptr);
            ASSERT_EQ(handle_set->size(), 14);
            success_count++;
            delete link_template;
        } catch (const exception& e) {
            cout << "Thread " << thread_id << " failed with error: " << e.what() << endl;
        }
    };

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(success_count, num_threads);

    // // Test non-existing pattern
    string metta = R"((Fake $x $y))";
    auto link_template = new LinkTemplateMetta(metta);
    auto handle_set = db->query_for_pattern(*link_template);
    delete link_template;
    EXPECT_EQ(handle_set->size(), 0);
}

TEST_F(MorkDBTest, ConcurrentQueryForTargets) {
    const int num_threads = 200;
    vector<thread> threads;
    atomic<int> success_count{0};
    string link_handle = exp_hash({"Similarity", "\"human\"", "\"monkey\""});

    auto worker = [&](int thread_id) {
        try {
            auto targets = db->query_for_targets(link_handle);
            ASSERT_NE(targets, nullptr);
            ASSERT_EQ(targets->size(), 3);
            success_count++;
        } catch (const exception& e) {
            cout << "Thread " << thread_id << " failed with error: " << e.what() << endl;
        }
    };

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(success_count, num_threads);

    // Test non-existing link
    auto targets = db->query_for_targets("10000000000000000000000000000000");
    EXPECT_EQ(targets, nullptr);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new MorkDBTestEnvironment());
    return RUN_ALL_TESTS();
}
