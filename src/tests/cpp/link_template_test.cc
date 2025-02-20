#include <cstdlib>

#include "AtomDBSingleton.h"
#include "HandlesAnswer.h"
#include "LinkTemplate.h"
#include "QueryNode.h"
#include "test_utils.h"
#include "gtest/gtest.h"

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
  QueryNodeServer<HandlesAnswer> server_node(server_node_id);

  AtomDBSingleton::init();
  string expression = "Expression";
  string symbol = "Symbol";

  Variable v1("v1");
  Variable v2("v2");
  Variable v3("v3");
  Node similarity(symbol, "Similarity");
  Node human(symbol, "\"human\"");

  LinkTemplate<3> link_template1("Expression", {&similarity, &human, &v1});
  link_template1.subsequent_id = server_node_id;
  link_template1.setup_buffers();
  // link_template1.fetch_links();
  Utils::sleep(1000);

  string monkey_handle =
      string(terminal_hash((char *)symbol.c_str(), (char *)"\"monkey\""));
  string chimp_handle =
      string(terminal_hash((char *)symbol.c_str(), (char *)"\"chimp\""));
  string ent_handle =
      string(terminal_hash((char *)symbol.c_str(), (char *)"\"ent\""));
  bool monkey_flag = false;
  bool chimp_flag = false;
  bool ent_flag = false;
  HandlesAnswer *query_answer;
  while ((query_answer = dynamic_cast<HandlesAnswer *>(
              server_node.pop_query_answer())) != NULL) {
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
