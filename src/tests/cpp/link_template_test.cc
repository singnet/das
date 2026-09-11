#include <cstdlib>
#include <memory>
#include <set>
#include <string>

#include "AtomDBAPITypes.h"
#include "AtomDBSingleton.h"
#include "Hasher.h"
#include "InMemoryDB.h"
#include "Link.h"
#include "LinkTemplate.h"
#include "Node.h"
#include "QueryAnswer.h"
#include "QueryNode.h"
#include "Terminal.h"
#include "TestAtomDBJsonConfig.h"
#include "gtest/gtest.h"
#include "test_utils.h"

using namespace atomdb;
using namespace query_engine;
using namespace query_element;

TEST(LinkTemplate, basics) {
    string server_node_id = "SERVER";
    QueryNodeServer server_node(server_node_id);

    AtomDBSingleton::init(test_atomdb_json_config());
    string expression = "Expression";
    string symbol = "Symbol";

    auto v1 = make_shared<Terminal>("v1");
    auto v2 = make_shared<Terminal>("v2");
    auto v3 = make_shared<Terminal>("v3");
    auto similarity = make_shared<Terminal>();
    similarity->handle = Hasher::node_handle(symbol, "Similarity");
    auto human = make_shared<Terminal>(symbol, "\"human\"");

    LinkTemplate link_template1("Expression", {similarity, human, v1}, "", 0.0, false, false, false);
    link_template1.build();
    link_template1.get_source_element()->subsequent_id = server_node_id;
    link_template1.get_source_element()->setup_buffers();
    Utils::sleep(2000);

    // Compare LinkTemplate and LinkSchema handles
    LinkSchema schema(expression, 3);
    schema.stack_node(symbol, "Similarity");
    schema.stack_node(symbol, "\"human\"");
    schema.stack_untyped_variable("v1");
    schema.build();
    EXPECT_EQ(schema.handle(), link_template1.get_handle());

    string monkey_handle = string(terminal_hash((char*) symbol.c_str(), (char*) "\"monkey\""));
    string chimp_handle = string(terminal_hash((char*) symbol.c_str(), (char*) "\"chimp\""));
    string ent_handle = string(terminal_hash((char*) symbol.c_str(), (char*) "\"ent\""));
    bool monkey_flag = false;
    bool chimp_flag = false;
    bool ent_flag = false;
    QueryAnswer* query_answer;
    while ((query_answer = dynamic_cast<QueryAnswer*>(server_node.pop_query_answer())) != NULL) {
        string var = string(query_answer->assignment.get("v1"));
        // EXPECT_TRUE(double_equals(query_answer->importance, 0.0));
        if (var == monkey_handle) {
            // TODO: perform extra checks
            monkey_flag = true;
        } else if (var == chimp_handle) {
            // TODO: perform extra checks
            chimp_flag = true;
        } else if (var == ent_handle) {
            // TODO: perform extra checks
            ent_flag = true;
        } else {
            FAIL();
        }
    }
    EXPECT_TRUE(monkey_flag);
    EXPECT_TRUE(chimp_flag);
    EXPECT_TRUE(ent_flag);
}

TEST(LinkTemplate, key_tokens) {
    EXPECT_THROW(
        { LinkTemplate link_template_1("Expression", {}, "", 0.0, false, false, false, "uid1"); },
        runtime_error);
    EXPECT_THROW(
        { LinkTemplate link_template_2("Expression", {}, "", 0.0, false, false, false, " uid1"); },
        runtime_error);
    EXPECT_THROW(
        { LinkTemplate link_template_3("Expression", {}, "", 0.0, false, false, false, "uid1  key1"); },
        runtime_error);
    EXPECT_THROW(
        { LinkTemplate link_template_4("Expression", {}, "", 0.0, false, false, false, "uid1 "); },
        runtime_error);
    LinkTemplate link_template1("Expression", {}, "", 0.0, false, false, false, "");
    LinkTemplate link_template2("Expression", {}, "", 0.0, false, false, false, "uid1 key1");
    LinkTemplate link_template3("Expression", {}, "", 0.0, false, false, false, "uid1 key1 uid2 key2");
}

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
