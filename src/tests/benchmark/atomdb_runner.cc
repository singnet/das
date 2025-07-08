#include <gtest/gtest.h>

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>

#include "AtomDB.h"
#include "RedisMongoDB.h"
#include "AtomDBCache.h"

using namespace std;


class AtomDBTestEnvironment : public ::testing::Environment {
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
        AtomDBSingleton::init();
    }
};

class AtomDBTest : public ::testing::Test {
   protected:
    void SetUp() override {
        auto atomdb = AtomDBSingleton::get_instance();
        db = dynamic_pointer_cast<RedisMongoDB>(atomdb);
        ASSERT_NE(db, nullptr) << "Failed to cast AtomDB to RedisMongoDB";
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

    shared_ptr<RedisMongoDB> db;
};

TEST_F(AtomDBTest, AddAtom) {
    // WIP
}

TEST_F(AtomDBTest, AddAtoms) {
    // WIP
}

struct Stats {
    atomic<size_t> total_ops{0};
    atomic<size_t> errors{0};
    atomic<double> total_latency_ms{0};
};

// -----------------------------------------------------
int main(const string& action, int iterations, bool very_connected, Stats& stats) {
    if (argc < 5) {
        "/src/scripts/run.sh benchmark "$CONCURRENCY" "$CACHE" "$action" "$METRICS""
        cerr << "Usage: " << argv[0]
             << " <concurrency> <cache> <action> <metrics> "
             << endl;
        exit(1);
    }

    string concurrency = string(argv[1]);
    string cache = string(argv[2]) == "enabled" ? "true" : "false";;
    string action = string(argv[3]);
    string metrics = string(argv[4]);


    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);
   
    return 0;
}
