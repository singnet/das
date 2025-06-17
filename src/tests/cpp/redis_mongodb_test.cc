#include <gtest/gtest.h>

#include <atomic>
#include <cstring>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "atomdb/AtomDBSingleton.h"
#include "atomdb/RedisMongoDB.h"
#include "expression_hasher.h"

using namespace atomdb;
using namespace std;

class RedisMongoDBTestEnvironment : public ::testing::Environment {
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
        AtomDBSingleton::init();
    }
};

class RedisMongoDBTest : public ::testing::Test {
   protected:
    void SetUp() override {
        auto atomdb = AtomDBSingleton::get_instance();
        db = dynamic_pointer_cast<RedisMongoDB>(atomdb);
        ASSERT_NE(db, nullptr) << "Failed to cast AtomDB to RedisMongoDB";
    }

    void TearDown() override {}

    shared_ptr<char> handle_ptr(string str) {
        size_t len = str.length() + 1;
        auto buffer = make_unique<char[]>(len);
        strcpy(buffer.get(), str.c_str());
        return shared_ptr<char>(buffer.release(), default_delete<char[]>());
    }

    shared_ptr<RedisMongoDB> db;
};

TEST_F(RedisMongoDBTest, ConcurrentQueryForPattern) {
    const int num_threads = 200;
    vector<thread> threads;
    atomic<int> success_count{0};

    vector<shared_ptr<char>> pattern_handles;
    for (int i = 0; i < num_threads; ++i) {
        pattern_handles.push_back(handle_ptr("e8ca47108af6d35664f8813e1f96c5fa"));
    }

    auto worker = [&](int thread_id) {
        try {
            auto handle_set = db->query_for_pattern(pattern_handles[thread_id]);
            ASSERT_NE(handle_set, nullptr);
            ASSERT_EQ(handle_set->size(), 3);
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

    EXPECT_EQ(success_count, num_threads) << "Not all threads completed successfully";
}

TEST_F(RedisMongoDBTest, ConcurrentQueryForDocument) {
    const int num_threads = 200;
    vector<thread> threads;
    atomic<int> success_count{0};

    string human_handle = string(terminal_hash((char*) "Symbol", (char*) "\"human\""));
    string monkey_handle = string(terminal_hash((char*) "Symbol", (char*) "\"monkey\""));
    string chimp_handle = string(terminal_hash((char*) "Symbol", (char*) "\"chimp\""));
    string ent_handle = string(terminal_hash((char*) "Symbol", (char*) "\"ent\""));

    vector<shared_ptr<char>> doc_handles;
    while (doc_handles.size() < num_threads) {
        doc_handles.push_back(handle_ptr(human_handle));
        doc_handles.push_back(handle_ptr(monkey_handle));
        doc_handles.push_back(handle_ptr(chimp_handle));
        doc_handles.push_back(handle_ptr(ent_handle));
    }

    auto worker = [&](int thread_id) {
        try {
            auto doc = db->get_atom_document(doc_handles[thread_id].get());
            if (doc != nullptr) {
                success_count++;
            }
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

    EXPECT_EQ(success_count, num_threads) << "Not all threads completed successfully";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new RedisMongoDBTestEnvironment());
    return RUN_ALL_TESTS();
}