#include <memory>
#include <set>
#include <string>

#include "AtomDBSingleton.h"
#include "InMemoryDB.h"
#include "Link.h"
#include "LinkTemplate.h"
#include "Node.h"
#include "QueryAnswer.h"
#include "QueryNode.h"
#include "Terminal.h"
#include "gtest/gtest.h"
#include "test_utils.h"

using namespace atomdb;
using namespace query_engine;
using namespace query_element;

// This case lives in its own binary because link_template_test.cc initializes the shared
// RedisMongoDB test AtomDB through AtomDBSingleton::provide(), and provide() accepts only one
// instance. This test needs a private InMemoryDB, so it cannot share that process.
//
// Move it back to link_template_test.cc when nodes can be deleted from the RedisMongoDB test
// AtomDB without affecting the other tests. It can then use that shared database instead of
// installing its own.

TEST(LinkTemplate, UniqueValueFilteringSetsAssignmentCompatibilityFlag) {
    auto db = make_shared<InMemoryDB>();
    AtomDBSingleton::provide(db);

    auto relation = make_shared<Node>("Symbol", "Relation");
    auto a = make_shared<Node>("Symbol", "\"a\"");
    auto b = make_shared<Node>("Symbol", "\"b\"");
    db->add_node(relation.get());
    db->add_node(a.get());
    db->add_node(b.get());
    string repeated_handle =
        db->add_link(new Link("Expression", {relation->handle(), a->handle(), a->handle()}));
    string unique_handle =
        db->add_link(new Link("Expression", {relation->handle(), a->handle(), b->handle()}));
    db->re_index_patterns(true);

    auto relation_terminal = make_shared<Terminal>();
    relation_terminal->handle = relation->handle();
    auto x = make_shared<Terminal>("x");
    auto y = make_shared<Terminal>("y");

    {
        string server_node_id = "NON_UNIQUE_VALUE_SERVER";
        QueryNodeServer server_node(server_node_id);
        LinkTemplate link_template("Expression", {relation_terminal, x, y}, "", 0.0, false, true, false);
        link_template.build();
        link_template.get_source_element()->subsequent_id = server_node_id;
        link_template.get_source_element()->setup_buffers();
        Utils::sleep(2000);

        set<string> handles;
        QueryAnswer* answer;
        while ((answer = dynamic_cast<QueryAnswer*>(server_node.pop_query_answer())) != nullptr) {
            handles.insert(*answer->get_handles_vector().begin());
            EXPECT_FALSE(answer->assignment.unique_assignment_flag);
            delete answer;
        }
        EXPECT_EQ(handles, (set<string>{repeated_handle, unique_handle}));
    }

    {
        string server_node_id = "UNIQUE_VALUE_SERVER";
        QueryNodeServer server_node(server_node_id);
        LinkTemplate link_template("Expression", {relation_terminal, x, y}, "", 0.0, false, true, true);
        link_template.build();
        link_template.get_source_element()->subsequent_id = server_node_id;
        link_template.get_source_element()->setup_buffers();
        Utils::sleep(2000);

        QueryAnswer* answer = dynamic_cast<QueryAnswer*>(server_node.pop_query_answer());
        ASSERT_NE(answer, nullptr);
        EXPECT_EQ(*answer->get_handles_vector().begin(), unique_handle);
        EXPECT_EQ(answer->assignment.get("x"), a->handle());
        EXPECT_EQ(answer->assignment.get("y"), b->handle());
        EXPECT_TRUE(answer->assignment.unique_assignment_flag);
        delete answer;
        EXPECT_EQ(server_node.pop_query_answer(), nullptr);
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    Utils::init_random(0);
    return RUN_ALL_TESTS();
}
