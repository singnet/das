#include <cstdlib>
#include <cstring>
#include "gtest/gtest.h"

#include "QueryAnswer.h"
#include "Utils.h"
#include "test_utils.h"

using namespace query_engine;
using namespace commons;

TEST(QueryAnswer, assignments_basics) {

    Assignment mapping0;

    // Tests assign()
    Assignment mapping1;
    EXPECT_TRUE(mapping1.assign("v1", "1"));
    EXPECT_TRUE(mapping1.assign("v2", "2"));
    EXPECT_TRUE(mapping1.assign("v2", "2"));
    EXPECT_FALSE(mapping1.assign("v2", "3"));
    Assignment mapping2;
    EXPECT_TRUE(mapping1.assign("v1", "1"));
    EXPECT_TRUE(mapping1.assign("v3", "3"));
    Assignment mapping3;
    EXPECT_TRUE(mapping3.assign("v1", "1"));
    EXPECT_TRUE(mapping3.assign("v2", "3"));

    // Tests get()
    EXPECT_TRUE(strcmp(mapping1.get("v1"), "1") == 0);
    EXPECT_TRUE(strcmp(mapping1.get("v2"), "2") == 0);
    EXPECT_TRUE(mapping1.get("blah") == NULL);
    EXPECT_TRUE(mapping1.get("v11") == NULL);
    EXPECT_TRUE(mapping1.get("v") == NULL);
    EXPECT_TRUE(mapping1.get("") == NULL);

    // Tests is_compatible()
    EXPECT_TRUE(mapping1.is_compatible(mapping0));
    EXPECT_TRUE(mapping2.is_compatible(mapping0));
    EXPECT_TRUE(mapping3.is_compatible(mapping0));
    EXPECT_TRUE(mapping0.is_compatible(mapping1));
    EXPECT_TRUE(mapping0.is_compatible(mapping2));
    EXPECT_TRUE(mapping0.is_compatible(mapping3));
    EXPECT_TRUE(mapping1.is_compatible(mapping2));
    EXPECT_TRUE(mapping2.is_compatible(mapping1));
    EXPECT_TRUE(mapping2.is_compatible(mapping3));
    EXPECT_TRUE(mapping3.is_compatible(mapping2));
    EXPECT_FALSE(mapping1.is_compatible(mapping3));
    EXPECT_FALSE(mapping3.is_compatible(mapping1));

    // Tests copy_from()
    Assignment mapping4;
    mapping4.copy_from(mapping1);
    EXPECT_TRUE(strcmp(mapping4.get("v1"), "1") == 0);
    EXPECT_TRUE(strcmp(mapping4.get("v2"), "2") == 0);
    EXPECT_TRUE(mapping4.is_compatible(mapping2));
    EXPECT_TRUE(mapping2.is_compatible(mapping4));
    EXPECT_FALSE(mapping4.is_compatible(mapping3));
    EXPECT_FALSE(mapping3.is_compatible(mapping4));

    // Tests add_assignments()
    mapping4.add_assignments(mapping1);
    mapping4.add_assignments(mapping2);
    EXPECT_TRUE(strcmp(mapping4.get("v1"), "1") == 0);
    EXPECT_TRUE(strcmp(mapping4.get("v2"), "2") == 0);
    EXPECT_TRUE(strcmp(mapping4.get("v3"), "3") == 0);
    EXPECT_TRUE(mapping1.is_compatible(mapping4));
    EXPECT_TRUE(mapping2.is_compatible(mapping4));
    EXPECT_FALSE(mapping3.is_compatible(mapping4));
    EXPECT_TRUE(mapping4.is_compatible(mapping1));
    EXPECT_TRUE(mapping4.is_compatible(mapping2));
    EXPECT_FALSE(mapping4.is_compatible(mapping3));

    // Tests to_string():
    EXPECT_TRUE(mapping0.to_string() != "");
    EXPECT_TRUE(mapping1.to_string() != "");
    EXPECT_TRUE(mapping4.to_string() != "");
}

TEST(QueryAnswer, query_answer_basics) {

    // Tests add_handle()
    QueryAnswer query_answer1("h1", 0);
    query_answer1.assignment.assign("v1", "1");
    EXPECT_EQ(query_answer1.handles_size, 1);
    EXPECT_TRUE(strcmp(query_answer1.handles[0], "h1") == 0);
    query_answer1.add_handle("hx");
    EXPECT_EQ(query_answer1.handles_size, 2);
    EXPECT_TRUE(strcmp(query_answer1.handles[0], "h1") == 0);
    EXPECT_TRUE(strcmp(query_answer1.handles[1], "hx") == 0);

    // Tests merge()
    QueryAnswer query_answer2("h2", 0);
    query_answer2.assignment.assign("v2", "2");
    query_answer2.add_handle("hx");
    query_answer2.merge(&query_answer1);
    EXPECT_EQ(query_answer2.handles_size, 3);
    EXPECT_TRUE(strcmp(query_answer2.handles[0], "h2") == 0);
    EXPECT_TRUE(strcmp(query_answer2.handles[1], "hx") == 0);
    EXPECT_TRUE(strcmp(query_answer2.handles[2], "h1") == 0);
    EXPECT_FALSE(query_answer2.assignment.assign("v1", "x"));
    EXPECT_FALSE(query_answer2.assignment.assign("v2", "x"));
    EXPECT_TRUE(query_answer2.assignment.assign("v3", "x"));

    // Tests copy()
    QueryAnswer *query_answer3 = QueryAnswer::copy(&query_answer2);
    EXPECT_EQ(query_answer3->handles_size, 3);
    EXPECT_TRUE(strcmp(query_answer3->handles[0], "h2") == 0);
    EXPECT_TRUE(strcmp(query_answer3->handles[1], "hx") == 0);
    EXPECT_TRUE(strcmp(query_answer3->handles[2], "h1") == 0);
    EXPECT_FALSE(query_answer3->assignment.assign("v1", "x"));
    EXPECT_FALSE(query_answer3->assignment.assign("v2", "x"));
    EXPECT_FALSE(query_answer3->assignment.assign("v3", "y"));
    EXPECT_TRUE(query_answer3->assignment.assign("v4", "x"));
}

void query_answers_equal(QueryAnswer *qa1, QueryAnswer *qa2) {
    EXPECT_TRUE(double_equals(qa1->importance, qa2->importance));
    EXPECT_EQ(qa1->to_string(), qa2->to_string());
}

TEST(QueryAnswer, tokenization) {

    unsigned int NUM_TESTS = 100000;
    unsigned int MAX_HANDLES = 5;
    unsigned int MAX_ASSIGNMENTS = 10;

    for (unsigned int test = 0; test < NUM_TESTS; test++) {
        unsigned int num_handles = (rand() % MAX_HANDLES) + 1;
        unsigned int num_assignments = (rand() % MAX_ASSIGNMENTS);
        QueryAnswer input(Utils::flip_coin() ? 1 : 0);
        for (unsigned int i = 0; i < num_handles; i++) {
            input.add_handle(strdup(random_handle().c_str()));
        }
        unsigned int label_count = 0;
        for (unsigned int i = 0; i < num_assignments; i++) {
            input.assignment.assign(
                strdup(sequential_label(label_count).c_str()),
                strdup(random_handle().c_str()));
        }

        query_answers_equal(&input, QueryAnswer::copy(&input));
        string token_string = input.tokenize();
        QueryAnswer output(0.0);
        output.untokenize(token_string);
        query_answers_equal(&input, &output);
    }
}
