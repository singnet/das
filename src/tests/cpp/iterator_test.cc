#include "Iterator.h"

#include <cstdlib>

#include "AtomDBSingleton.h"
#include "LinkTemplate.h"
#include "QueryAnswer.h"
#include "QueryNode.h"
#include "Terminal.h"
#include "TestConfig.h"
#include "gtest/gtest.h"
#include "test_utils.h"

using namespace query_engine;
using namespace query_element;
using namespace query_node;

class TestQueryElement : public QueryElement {
   public:
    TestQueryElement(const string& id) { this->id = id; }
    void setup_buffers() {}
    void graceful_shutdown() {}
};

TEST(Iterator, basics) {
    string client_id = "no_query_element";
    auto dummy = make_shared<TestQueryElement>(client_id);
    Iterator query_answer_iterator(dummy);
    string server_id = query_answer_iterator.id;
    EXPECT_FALSE(server_id == "");
    QueryNodeClient client_node(client_id, query_answer_iterator.id);

    EXPECT_FALSE(query_answer_iterator.finished());

    QueryAnswer* qa;
    QueryAnswer qa0("h0", 0.0);
    QueryAnswer qa1("h1", 0.1);
    QueryAnswer qa2("h2", 0.2);

    client_node.add_query_answer(&qa0);
    client_node.add_query_answer(&qa1);
    Utils::sleep(1000);

    EXPECT_FALSE(query_answer_iterator.finished());
    qa = dynamic_cast<QueryAnswer*>(query_answer_iterator.pop());
    EXPECT_TRUE(strcmp(qa->handles[0], "h0") == 0);
    EXPECT_TRUE(double_equals(qa->importance, 0.0));

    EXPECT_FALSE(query_answer_iterator.finished());
    qa = dynamic_cast<QueryAnswer*>(query_answer_iterator.pop());
    EXPECT_TRUE(strcmp(qa->handles[0], "h1") == 0);
    EXPECT_TRUE(double_equals(qa->importance, 0.1));

    qa = dynamic_cast<QueryAnswer*>(query_answer_iterator.pop());
    EXPECT_TRUE(qa == NULL);
    EXPECT_FALSE(query_answer_iterator.finished());

    client_node.add_query_answer(&qa2);
    EXPECT_FALSE(client_node.is_query_answers_finished());
    EXPECT_FALSE(query_answer_iterator.finished());
    client_node.query_answers_finished();
    EXPECT_TRUE(client_node.is_query_answers_finished());
    EXPECT_FALSE(query_answer_iterator.finished());
    Utils::sleep(1000);

    EXPECT_FALSE(query_answer_iterator.finished());
    qa = dynamic_cast<QueryAnswer*>(query_answer_iterator.pop());
    EXPECT_TRUE(strcmp(qa->handles[0], "h2") == 0);
    EXPECT_TRUE(double_equals(qa->importance, 0.2));
    EXPECT_TRUE(query_answer_iterator.finished());
}

TEST(Iterator, link_template_integration) {
    TestConfig::load_environment();

    AtomDBSingleton::init();
    string expression = "Expression";
    string symbol = "Symbol";

    auto v1 = make_shared<Terminal>("v1");
    auto v2 = make_shared<Terminal>("v2");
    auto v3 = make_shared<Terminal>("v3");
    auto similarity = make_shared<Terminal>(symbol, "Similarity");
    auto human = make_shared<Terminal>(symbol, "\"human\"");

    LinkTemplate* link_template = new LinkTemplate("Expression", {similarity, human, v1}, "", false);
    link_template->build();
    Iterator query_answer_iterator(link_template->get_source_element());

    string monkey_handle = string(terminal_hash((char*) symbol.c_str(), (char*) "\"monkey\""));
    string chimp_handle = string(terminal_hash((char*) symbol.c_str(), (char*) "\"chimp\""));
    string ent_handle = string(terminal_hash((char*) symbol.c_str(), (char*) "\"ent\""));
    bool monkey_flag = false;
    bool chimp_flag = false;
    bool ent_flag = false;
    QueryAnswer* query_answer;
    while (!query_answer_iterator.finished()) {
        query_answer = dynamic_cast<QueryAnswer*>(query_answer_iterator.pop());
        if (query_answer != NULL) {
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
    }
    EXPECT_TRUE(monkey_flag);
    EXPECT_TRUE(chimp_flag);
    EXPECT_TRUE(ent_flag);
}
