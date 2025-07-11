#include <cstdlib>

#include "And.h"
#include "AtomDBSingleton.h"
#include "Iterator.h"
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

    AtomDBSingleton::init();

    const string expression = "Expression";
    const string symbol = "Symbol";

    auto v1 = make_shared<Terminal>("v1");
    auto v2 = make_shared<Terminal>("v2");
    auto similarity = make_shared<Terminal>(symbol, "Similarity");
    auto odd_link = make_shared<Terminal>(symbol, "OddLink");

    LinkTemplate* inner_template_ptr = new LinkTemplate(expression, {similarity, v1, v2}, "", false);
    shared_ptr<LinkTemplate> inner_template(inner_template_ptr);
    LinkTemplate* outter_template_ptr =
        new LinkTemplate(expression, {odd_link, inner_template}, "", false);
    outter_template_ptr->build();
    Iterator iterator(outter_template_ptr->get_source_element());

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

    auto v1 = make_shared<Terminal>("v1");
    auto v2 = make_shared<Terminal>("v2");
    auto similarity = make_shared<Terminal>(symbol, "Similarity");
    auto odd_link = make_shared<Terminal>(symbol, "OddLink");
    auto human = make_shared<Terminal>(symbol, "\"human\"");

    LinkTemplate* inner_template_ptr = new LinkTemplate(expression, {similarity, v1, v2}, "", false);
    shared_ptr<LinkTemplate> inner_template(inner_template_ptr);
    LinkTemplate* outter_template = new LinkTemplate(expression, {odd_link, inner_template}, "", false);
    outter_template->build();
    LinkTemplate* human_template = new LinkTemplate(expression, {similarity, v1, human}, "", false);
    human_template->build();
    auto and_operator = make_shared<And<2>>(array<shared_ptr<QueryElement>, 2>(
        {human_template->get_source_element(), outter_template->get_source_element()}));
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
