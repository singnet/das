#include "MorkWrapper.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <fstream>
#include <iostream>
#include <memory>
#include <thread>
#include <unordered_set>
#include <vector>

#include "Atom.h"
#include "AtomDBAPITypes.h"
#include "AtomDBSingleton.h"
#include "BoundedSharedQueue.h"
#include "ContextLoader.h"
#include "DatabaseOrchestrator.h"
#include "DatabaseTypes.h"
#include "DedicatedThread.h"
#include "Logger.h"
#include "Node.h"
#include "Processor.h"
#include "TestAtomDBJsonConfig.h"

using namespace std;
using namespace atomdb;
using namespace db_adapter;
using namespace atoms;
using namespace processor;

class MorkWrapperTest : public ::testing::Test {
   protected:
    string TEST_HOST = "localhost";
    int TEST_PORT = 40022;

    string INVALID_HOST = "invalid.host";
    int INVALID_PORT = 99999;

    void SetUp() override{};

    void TearDown() override { this->conn->stop(); };

    vector<shared_ptr<Atom>> read_atoms_from_queue(shared_ptr<BoundedSharedQueue> q) {
        vector<shared_ptr<Atom>> atoms;

        while (true) {
            if (q->empty()) break;
            void* raw_ptr = q->dequeue();
            std::queue<shared_ptr<Atom>>* batch_queue =
                static_cast<std::queue<shared_ptr<Atom>>*>(raw_ptr);

            if (batch_queue != nullptr) {
                while (!batch_queue->empty()) {
                    shared_ptr<Atom> atom = batch_queue->front();
                    batch_queue->pop();
                    if (atom != nullptr) {
                        atoms.push_back(atom);
                    }
                }
                delete batch_queue;
            }
        }

        return atoms;
    };

    shared_ptr<MorkWrapper> create_wrapper(shared_ptr<BoundedSharedQueue> queue = nullptr) {
        if (!queue) {
            queue = make_shared<BoundedSharedQueue>();
        }
        auto conn = create_db_connection();
        return make_shared<MorkWrapper>(*conn, queue);
    };

    shared_ptr<MorkConnection> create_db_connection() {
        this->conn = make_shared<MorkConnection>("mork-test-conn", TEST_HOST, TEST_PORT);
        this->conn->setup();
        this->conn->start();
        return this->conn;
    };

    shared_ptr<MorkConnection> conn;
};

TEST_F(MorkWrapperTest, MapNoResults) {
    auto queue = make_shared<BoundedSharedQueue>();
    auto wrapper = create_wrapper(queue);

    EXPECT_THROW({ wrapper->map("(Fake $F)"); }, runtime_error);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
