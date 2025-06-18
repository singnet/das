#include <gtest/gtest.h>

#include <atomic>
#include <cstring>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "AtomDBSingleton.h"
#include "RedisMongoDB.h"
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

    string exp_hash(vector<string> targets) {
        char* symbol = (char*) "Symbol";
        char** targets_handles = new char*[targets.size()];
        for (size_t i = 0; i < targets.size(); i++) {
            targets_handles[i] = terminal_hash(symbol, (char*) targets[i].c_str());
        }
        char* expression = named_type_hash((char*) "Expression");
        return string(expression_hash(expression, targets_handles, targets.size()));
    }

    shared_ptr<RedisMongoDB> db;
};

TEST_F(RedisMongoDBTest, ConcurrentQueryForPattern) {
    const int num_threads = 200;
    vector<thread> threads;
    atomic<int> success_count{0};

    auto worker = [&](int thread_id) {
        try {
            auto handle_set = db->query_for_pattern(handle_ptr("e8ca47108af6d35664f8813e1f96c5fa"));
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

    EXPECT_EQ(success_count, num_threads);

    // Test non-existing pattern
    auto non_existing_pattern = handle_ptr("00000000000000000000000000000000");
    auto handle_set = db->query_for_pattern(non_existing_pattern);
    EXPECT_EQ(handle_set->size(), 0);
}

TEST_F(RedisMongoDBTest, ConcurrentQueryForTargets) {
    const int num_threads = 200;
    vector<thread> threads;
    atomic<int> success_count{0};

    auto worker = [&](int thread_id) {
        try {
            auto targets = db->query_for_targets(handle_ptr("68ea071c32d4dbf0a7d8e8e00f2fb823"));
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
    auto targets = db->query_for_targets(handle_ptr("00000000000000000000000000000000"));
    EXPECT_EQ(targets, nullptr);
}

TEST_F(RedisMongoDBTest, ConcurrentGetAtomDocument) {
    const int num_threads = 200;
    vector<thread> threads;
    atomic<int> success_count{0};

    char* human_handle = terminal_hash((char*) "Symbol", (char*) "\"human\"");
    char* monkey_handle = terminal_hash((char*) "Symbol", (char*) "\"monkey\"");
    char* chimp_handle = terminal_hash((char*) "Symbol", (char*) "\"chimp\"");
    char* ent_handle = terminal_hash((char*) "Symbol", (char*) "\"ent\"");

    vector<char*> doc_handles;
    while (doc_handles.size() < num_threads) {
        doc_handles.push_back(human_handle);
        doc_handles.push_back(monkey_handle);
        doc_handles.push_back(chimp_handle);
        doc_handles.push_back(ent_handle);
    }

    auto worker = [&](int thread_id) {
        try {
            auto doc = db->get_atom_document(doc_handles[thread_id]);
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

    EXPECT_EQ(success_count, num_threads);

    // Test non-existing handle
    char* non_existing_handle = terminal_hash((char*) "Symbol", (char*) "\"non-existing\"");
    auto doc = db->get_atom_document(non_existing_handle);
    EXPECT_EQ(doc, nullptr);
}

TEST_F(RedisMongoDBTest, ConcurrentGetAtomDocuments) {
    const int num_threads = 200;
    vector<thread> threads;
    atomic<int> success_count{0};

    string handle_1 = exp_hash({"Similarity", "\"human\"", "\"monkey\""});
    string handle_2 = exp_hash({"Similarity", "\"human\"", "\"chimp\""});
    string handle_3 = exp_hash({"Similarity", "\"human\"", "\"ent\""});
    string handle_4 = exp_hash({"Similarity", "\"chimp\"", "\"monkey\""});

    auto worker = [&](int thread_id) {
        try {
            auto docs = db->get_atom_documents({handle_1, handle_2, handle_3, handle_4}, {"targets"});
            if (docs.size() == 4) {
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

    EXPECT_EQ(success_count, num_threads);

    // Test non-existing handle
    string non_existing_handle = exp_hash({"Similarity", "\"non\"", "\"existing\""});
    auto docs = db->get_atom_documents({non_existing_handle}, {"targets"});
    EXPECT_EQ(docs.size(), 0);
}

TEST_F(RedisMongoDBTest, ConcurrentLinkExists) {
    const int num_threads = 200;
    vector<thread> threads;
    atomic<int> success_count{0};

    auto worker = [&](int thread_id) {
        try {
            auto link_exists = db->link_exists("68ea071c32d4dbf0a7d8e8e00f2fb823");
            ASSERT_TRUE(link_exists);
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
    auto link_exists = db->link_exists("00000000000000000000000000000000");
    EXPECT_FALSE(link_exists);
}

TEST_F(RedisMongoDBTest, ConcurrentLinksExist) {
    const int num_threads = 200;
    vector<thread> threads;
    atomic<int> success_count{0};

    auto worker = [&](int thread_id) {
        try {
            auto link_exists = db->links_exist({"68ea071c32d4dbf0a7d8e8e00f2fb823",
                                                "00000000000000000000000000000000",
                                                "7ec8526b8c8f15a6ac55273fedbf694f"});
            ASSERT_EQ(link_exists.size(), 2);
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
    auto link_exists = db->links_exist({"00000000000000000000000000000000",
                                        "00000000000000000000000000000001",
                                        "00000000000000000000000000000002"});
    EXPECT_EQ(link_exists.size(), 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new RedisMongoDBTestEnvironment());
    return RUN_ALL_TESTS();
}
