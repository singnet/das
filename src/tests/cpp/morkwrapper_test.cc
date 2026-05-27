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
                        LOG_INFO("Read atom from queue: " << atom->to_string());
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

TEST_F(MorkWrapperTest, MapNoResults) {
    auto queue = make_shared<BoundedSharedQueue>();
    auto wrapper = create_wrapper(queue);

    EXPECT_THROW({ wrapper->map("(Fake $F)"); }, runtime_error);
}

TEST_F(MorkWrapperTest, MapSuccess) {
    auto queue = make_shared<BoundedSharedQueue>();
    auto wrapper = create_wrapper(queue);

    vector<string> metta_queries = {
        "(Similarity \"ent\" $h)",
        "(Inheritance \"human\" $m)",
    };

    for (const auto& metta_query : metta_queries) {
        try {
            wrapper->map(metta_query);
            LOG_DEBUG("Successfully mapped MORK query: " << metta_query);
        } catch (const exception& e) {
            LOG_ERROR("Error mapping MORK query: " << metta_query);
        }
    }

    vector<shared_ptr<Atom>> atoms = read_atoms_from_queue(queue);

    size_t expected_atom_count = 8;
    ASSERT_EQ(atoms.size(), expected_atom_count);

    vector<string> ordered_handles;
    vector<shared_ptr<Node>> nodes;
    vector<shared_ptr<Link>> links;
    for (const auto& atom : atoms) {
        ordered_handles.push_back(atom->to_string());
        if (atom->arity() == 0) {
            nodes.push_back(dynamic_pointer_cast<Node>(atom));
        } else {
            links.push_back(dynamic_pointer_cast<Link>(atom));
        }
    }

    ASSERT_EQ(nodes.size(), 6);
    ASSERT_EQ(links.size(), 2);

    shared_ptr<Node> node_human = make_shared<Node>("Symbol", "\"human\"");
    shared_ptr<Node> node_ent = make_shared<Node>("Symbol", "\"ent\"");
    shared_ptr<Node> node_mammal = make_shared<Node>("Symbol", "\"mammal\"");
    shared_ptr<Node> node_similarity = make_shared<Node>("Symbol", "Similarity");
    shared_ptr<Node> node_inheritance = make_shared<Node>("Symbol", "Inheritance");
    shared_ptr<Link> link_similarity_ent_human = make_shared<Link>(
        "Expression",
        vector<string>{node_similarity->handle(), node_ent->handle(), node_human->handle()},
        true);
    shared_ptr<Link> link_inheritance_human_mammal = make_shared<Link>(
        "Expression",
        vector<string>{node_inheritance->handle(), node_human->handle(), node_mammal->handle()},
        true);

    vector<string> expected_ordered_handle = {node_similarity->to_string(),
                                              node_ent->to_string(),
                                              node_human->to_string(),
                                              link_similarity_ent_human->to_string(),
                                              node_inheritance->to_string(),
                                              node_human->to_string(),
                                              node_mammal->to_string(),
                                              link_inheritance_human_mammal->to_string()};

    ASSERT_EQ(ordered_handles, expected_ordered_handle);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
