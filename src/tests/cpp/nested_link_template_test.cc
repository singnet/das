#include <cstdlib>

#include "AtomDBSingleton.h"
#include "QueryAnswer.h"
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

    auto v1 = make_shared<Variable>("v1");
    auto v2 = make_shared<Variable>("v2");
    auto similarity = make_shared<Node>(symbol, "Similarity");
    auto odd_link = make_shared<Node>(symbol, "OddLink");

    auto inner_template = make_shared<LinkTemplate<3>>(
        expression, array<shared_ptr<QueryElement>, 3>({similarity, v1, v2}));
    auto outter_template = make_shared<LinkTemplate<2>>(
        expression, array<shared_ptr<QueryElement>, 2>({odd_link, inner_template}));
    Iterator iterator(outter_template);

    QueryAnswer* query_answer;
    unsigned int count = 0;
    while (!iterator.finished()) {
        if ((query_answer = dynamic_cast<QueryAnswer*>(iterator.pop())) == NULL) {
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

    auto v1 = make_shared<Variable>("v1");
    auto v2 = make_shared<Variable>("v2");
    auto similarity = make_shared<Node>(symbol, "Similarity");
    auto odd_link = make_shared<Node>(symbol, "OddLink");
    auto human = make_shared<Node>(symbol, "\"human\"");

    auto inner_template = make_shared<LinkTemplate<3>>(
        expression, array<shared_ptr<QueryElement>, 3>({similarity, v1, v2}));
    auto outter_template = make_shared<LinkTemplate<2>>(
        expression, array<shared_ptr<QueryElement>, 2>({odd_link, inner_template}));
    auto human_template = make_shared<LinkTemplate<3>>(
        expression, array<shared_ptr<QueryElement>, 3>({similarity, v1, human}));
    auto and_operator =
        make_shared<And<2>>(array<shared_ptr<QueryElement>, 2>({human_template, outter_template}));

    Iterator iterator(and_operator);

    QueryAnswer* query_answer;
    unsigned int count = 0;
    while (!iterator.finished()) {
        if ((query_answer = dynamic_cast<QueryAnswer*>(iterator.pop())) == NULL) {
            Utils::sleep();
        } else {
            // EXPECT_TRUE(double_equals(query_answer->importance, 0.0));
            count++;
        }
    }
    EXPECT_EQ(count, 1);
}
