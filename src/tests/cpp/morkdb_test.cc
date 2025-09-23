#include "MorkDB.h"

#include <gtest/gtest.h>

#include <atomic>
#include <cstring>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "AtomDBSingleton.h"
#include "LinkSchema.h"

using namespace atomdb;
using namespace atoms;
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

static string expression = "Expression";
static string symbol = "Symbol";
static string link_template = "LINK_TEMPLATE";
static string node = "NODE";
static string variable = "VARIABLE";
static string inheritance = "Inheritance";
static string similarity = "Similarity";
static string human = "\"human\"";
static string monkey = "\"monkey\"";
static string chimp = "\"chimp\"";
static string rhino = "\"rhino\"";
static string mammal = "\"mammal\"";

TEST_F(MorkDBTest, QueryForPattern) {
    string handle_1 = exp_hash({inheritance, human, mammal});
    string handle_2 = exp_hash({inheritance, monkey, mammal});
    string handle_3 = exp_hash({inheritance, chimp, mammal});
    string handle_4 = exp_hash({inheritance, rhino, mammal});

    // clang-format off
    LinkSchema link_schema({
        link_template, expression, "3", 
            node, symbol, inheritance, 
            variable, "$x",
            node, symbol, mammal});
    // clang-format on

    auto result = db->query_for_pattern(link_schema);

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
    EXPECT_EQ(db->query_for_targets(link_handle), nullptr);
}

TEST_F(MorkDBTest, ConcurrentQueryForPattern) {
    const int num_threads = 200;
    vector<thread> threads;
    atomic<int> success_count{0};

    auto worker = [&](int thread_id) {
        try {
            // clang-format off
            LinkSchema link_schema({
                link_template, expression, "3", 
                    node, symbol, similarity, 
                    variable, "$x",
                    variable, "$y"});
            // clang-format on
            auto handle_set = db->query_for_pattern(link_schema);
            ASSERT_NE(handle_set, nullptr);
            ASSERT_EQ(handle_set->size(), 14);
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

    // // Test non-existing pattern
    // clang-format off
    LinkSchema link_schema({
        link_template, expression, "3", 
            node, symbol, "Fake", 
            variable, "$x",
            variable, "$y"});
    // clang-format on
    auto handle_set = db->query_for_pattern(link_schema);
    EXPECT_EQ(handle_set->size(), 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new MorkDBTestEnvironment());
    return RUN_ALL_TESTS();
}
