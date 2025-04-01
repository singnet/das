#include <cstdlib>

#include "AtomDBSingleton.h"
#include "HandlesAnswer.h"
#include "LinkTemplate.h"
#include "QueryNode.h"
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

    AtomDBSingleton::init();

    string expression = "Expression";
    string symbol = "Symbol";

    auto v1 = new Variable("v1");
    auto v2 = new Variable("v2");
    auto similarity = new Node(symbol, "Similarity");
    auto odd_link = new Node(symbol, "OddLink");

    auto inner_template = new LinkTemplate<3>(expression, {similarity, v1, v2});
    auto outter_template = new LinkTemplate<2>(expression, {odd_link, inner_template});
    Iterator<HandlesAnswer> iterator(outter_template);

    HandlesAnswer* query_answer;
    unsigned int count = 0;
    while (!iterator.finished()) {
        if ((query_answer = dynamic_cast<HandlesAnswer*>(iterator.pop())) == NULL) {
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

    auto v1 = new Variable("v1");
    auto v2 = new Variable("v2");
    auto similarity = new Node(symbol, "Similarity");
    auto odd_link = new Node(symbol, "OddLink");
    auto human = new Node(symbol, "\"human\"");

    auto inner_template = new LinkTemplate<3>(expression, {similarity, v1, v2});
    auto outter_template = new LinkTemplate<2>(expression, {odd_link, inner_template});
    auto human_template = new LinkTemplate<3>(expression, {similarity, v1, human});
    auto and_operator = new And<2>({human_template, outter_template});

    Iterator<HandlesAnswer> iterator(and_operator);

    HandlesAnswer* query_answer;
    unsigned int count = 0;
    while (!iterator.finished()) {
        if ((query_answer = dynamic_cast<HandlesAnswer*>(iterator.pop())) == NULL) {
            Utils::sleep();
        } else {
            // EXPECT_TRUE(double_equals(query_answer->importance, 0.0));
            count++;
        }
    }
    EXPECT_EQ(count, 1);
}
