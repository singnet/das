#include <cstdlib>

#include "AtomDBSingleton.h"
#include "Hasher.h"
#include "LinkTemplate.h"
#include "QueryAnswer.h"
#include "QueryNode.h"
#include "Terminal.h"
#include "gtest/gtest.h"
#include "test_utils.h"

using namespace query_engine;
using namespace query_element;

TEST(LinkTemplate, basics) {
    setenv("DAS_REDIS_HOSTNAME", "localhost", 1);
    setenv("DAS_REDIS_PORT", "29000", 1);
    setenv("DAS_USE_REDIS_CLUSTER", "false", 1);
    setenv("DAS_MONGODB_HOSTNAME", "localhost", 1);
    setenv("DAS_MONGODB_PORT", "28000", 1);
    setenv("DAS_MONGODB_USERNAME", "dbadmin", 1);
    setenv("DAS_MONGODB_PASSWORD", "dassecret", 1);

    string server_node_id = "SERVER";
    QueryNodeServer server_node(server_node_id);

    AtomDBSingleton::init();
    string expression = "Expression";
    string symbol = "Symbol";

    auto v1 = make_shared<Terminal>("v1");
    auto v2 = make_shared<Terminal>("v2");
    auto v3 = make_shared<Terminal>("v3");
    auto similarity = make_shared<Terminal>();
    similarity->handle = Hasher::node_handle(symbol, "Similarity");
    auto human = make_shared<Terminal>(symbol, "\"human\"");

    LinkTemplate link_template1("Expression", {similarity, human, v1}, "", false);
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
