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
#include "MettaParser.h"
#include "MettaParserActions.h"
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

    unordered_set<string> read_atoms_from_queue(shared_ptr<BoundedSharedQueue> q) {
        unordered_set<string> atom_handles;

        while (true) {
            if (q->empty()) break;
            void* raw_ptr = q->dequeue();
            std::queue<Atom*>* batch_queue = static_cast<std::queue<Atom*>*>(raw_ptr);

            if (batch_queue != nullptr) {
                while (!batch_queue->empty()) {
                    Atom* atom = batch_queue->front();
                    batch_queue->pop();
                    if (atom != nullptr) {
                        if (atom_handles.find(atom->handle()) == atom_handles.end()) {
                            atom_handles.insert(atom->handle());
                        }
                    }
                    // delete atom;
                }
                delete batch_queue;
            }
        }

        return atom_handles;
    };

    shared_ptr<MorkWrapper> create_wrapper(shared_ptr<BoundedSharedQueue> queue = nullptr) {
        if (!queue) {
            queue = make_shared<BoundedSharedQueue>();
        }
        auto conn = create_db_connection();
        return make_shared<MorkWrapper>(conn, queue);
    };

    shared_ptr<MorkConnection> create_db_connection() {
        this->conn = make_shared<MorkConnection>("mork-test-conn", TEST_HOST, TEST_PORT);
        this->conn->setup();
        this->conn->start();
        return this->conn;
    };

    shared_ptr<MorkConnection> conn;
};

TEST_F(MorkWrapperTest, MapInvalidQuery) {
    auto queue = make_shared<BoundedSharedQueue>();
    auto wrapper = create_wrapper(queue);

    EXPECT_THROW({ wrapper->map("(Fake $F)"); }, runtime_error);
}

TEST_F(MorkWrapperTest, MapNoAtomsMapped) {
    auto queue = make_shared<BoundedSharedQueue>();
    auto wrapper = create_wrapper(queue);

    string metta_query = "(Similarity \"ent\" $y)";
    EXPECT_THROW({ wrapper->map(metta_query); }, runtime_error);
}

TEST_F(MorkWrapperTest, Map) {
    auto queue = make_shared<BoundedSharedQueue>();
    auto wrapper = create_wrapper(queue);

    vector<string> metta_queries = {"(Similarity \"ent\" $x)"};

    shared_ptr<MettaParserActions> pa = make_shared<MettaParserActions>();
    bool error_occurred = false;
    int count = 0;
    for (const auto& metta_query : metta_queries) {
        count++;
        try {
            wrapper->map(metta_query);
        } catch (const exception& e) {
            LOG_ERROR("Erro na linha [" << count << "] expressao: [" << metta_query << "] - "
                                        << e.what());
            error_occurred = true;
        }
        if (!error_occurred) {
            LOG_INFO("Successfully mapped MORK query: " << metta_query);
        } else {
            LOG_ERROR("Error mapping MORK query: " << metta_query);
        }
        error_occurred = false;
    }

    LOG_INFO("Mapped " << count << " MORK expressions");

    unordered_set<string> atoms = read_atoms_from_queue(queue);

    size_t expected_atom_count = 3;
    ASSERT_EQ(atoms.size(), expected_atom_count);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
