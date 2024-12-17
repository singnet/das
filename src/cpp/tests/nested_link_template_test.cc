#include <cstdlib>
#include "gtest/gtest.h"

#include "QueryNode.h"
#include "AtomDB.h"
#include "LinkTemplate.h"
#include "AtomDBSingleton.h"
#include "test_utils.h"

using namespace query_engine;
using namespace query_element;

TEST(LinkTemplate, basics) {
    
    setenv("DAS_REDIS_HOSTNAME", "ninjato", 1);
    setenv("DAS_REDIS_PORT", "29000", 1);
    setenv("DAS_USE_REDIS_CLUSTER", "false", 1);
    setenv("DAS_MONGODB_HOSTNAME", "ninjato", 1);
    setenv("DAS_MONGODB_PORT", "28000", 1);
    setenv("DAS_MONGODB_USERNAME", "dbadmin", 1);
    setenv("DAS_MONGODB_PASSWORD", "dassecret", 1);

    AtomDBSingleton::init();
    string expression = "Expression";
    string symbol = "Symbol";

    Variable v1("v1");
    Variable v2("v2");
    Variable v3("v3");
    Node similarity(symbol, "Similarity");
    Node odd_link(symbol, "OddLink");

    LinkTemplate<3> inner_template(expression, {&similarity, &v1, &v2});
    LinkTemplate<2> outter_template(expression, {&odd_link, &inner_template});

    Iterator iterator(&outter_template);

    QueryAnswer *query_answer;
    unsigned int count = 0;
    while (! iterator.finished()) {
        if ((query_answer = iterator.pop()) == NULL) {
            Utils::sleep();
        } else {
            EXPECT_TRUE(double_equals(query_answer->importance, 0.0));
            count++;
        }
    }
    EXPECT_EQ(count, 8);
}

TEST(LinkTemplate, nested_variables) {
    
    string expression = "Expression";
    string symbol = "Symbol";

    Variable v1("v1");
    Variable v2("v2");
    Variable v3("v3");
    Node similarity(symbol, "Similarity");
    Node odd_link(symbol, "OddLink");
    Node human(symbol, "\"human\"");

    LinkTemplate<3> inner_template(expression, {&similarity, &v1, &v2});
    LinkTemplate<2> outter_template(expression, {&odd_link, &inner_template});
    LinkTemplate<3> human_template(expression, {&similarity, &v1, &human});
    And<2> and_operator({&human_template, &outter_template});

    Iterator iterator(&and_operator);

    QueryAnswer *query_answer;
    unsigned int count = 0;
    while (! iterator.finished()) {
        if ((query_answer = iterator.pop()) == NULL) {
            Utils::sleep();
        } else {
            //EXPECT_TRUE(double_equals(query_answer->importance, 0.0));
            count++;
        }
    }
    EXPECT_EQ(count, 1);
}
