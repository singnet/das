#include <cstdlib>
#include <cstring>
#include <unordered_set>

#include "QueryAnswer.h"
#include "Utils.h"
#include "gtest/gtest.h"
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
    EXPECT_TRUE(mapping2.assign("v1", "1"));
    EXPECT_TRUE(mapping2.assign("v3", "3"));
    Assignment mapping3;
    EXPECT_TRUE(mapping3.assign("v1", "1"));
    EXPECT_TRUE(mapping3.assign("v2", "3"));

    // Tests get()
    EXPECT_TRUE(mapping1.get("v1") == "1");
    EXPECT_TRUE(mapping1.get("v2") == "2");
    EXPECT_TRUE(mapping1.get("blah") == "");
    EXPECT_TRUE(mapping1.get("v11") == "");
    EXPECT_TRUE(mapping1.get("v") == "");
    EXPECT_TRUE(mapping1.get("") == "");

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
    EXPECT_TRUE(mapping4.get("v1") == "1");
    EXPECT_TRUE(mapping4.get("v2") == "2");
    EXPECT_TRUE(mapping4.is_compatible(mapping2));
    EXPECT_TRUE(mapping2.is_compatible(mapping4));
    EXPECT_FALSE(mapping4.is_compatible(mapping3));
    EXPECT_FALSE(mapping3.is_compatible(mapping4));

    // Tests add_assignments()
    mapping4.add_assignments(mapping1);
    mapping4.add_assignments(mapping2);
    EXPECT_TRUE(mapping4.get("v1") == "1");
    EXPECT_TRUE(mapping4.get("v2") == "2");
    EXPECT_TRUE(mapping4.get("v3") == "3");
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

TEST(QueryAnswer, unique_assignment_flag) {

    Assignment mapping1(true);
    EXPECT_TRUE(mapping1.assign("v1", "1"));
    EXPECT_TRUE(mapping1.assign("v2", "2"));
    Assignment mapping2(true);
    EXPECT_TRUE(mapping2.assign("v1", "1"));
    EXPECT_TRUE(mapping2.assign("v3", "3"));
    Assignment mapping3(true);
    EXPECT_TRUE(mapping3.assign("v1", "1"));
    EXPECT_TRUE(mapping3.assign("v2", "3"));
    Assignment mapping4(true);
    EXPECT_TRUE(mapping4.assign("v4", "4"));
    EXPECT_TRUE(mapping4.assign("v1", "1"));

    EXPECT_TRUE(mapping1.is_compatible(mapping1));
    EXPECT_TRUE(mapping1.is_compatible(mapping2));
    EXPECT_FALSE(mapping1.is_compatible(mapping3));
    EXPECT_TRUE(mapping1.is_compatible(mapping4));

    EXPECT_TRUE(mapping2.is_compatible(mapping1));
    EXPECT_TRUE(mapping2.is_compatible(mapping2));
    EXPECT_FALSE(mapping2.is_compatible(mapping3));
    EXPECT_TRUE(mapping2.is_compatible(mapping4));

    EXPECT_FALSE(mapping3.is_compatible(mapping1));
    EXPECT_FALSE(mapping3.is_compatible(mapping2));
    EXPECT_TRUE(mapping3.is_compatible(mapping3));
    EXPECT_TRUE(mapping3.is_compatible(mapping4));

    EXPECT_TRUE(mapping4.is_compatible(mapping1));
    EXPECT_TRUE(mapping4.is_compatible(mapping2));
    EXPECT_TRUE(mapping4.is_compatible(mapping3));
    EXPECT_TRUE(mapping4.is_compatible(mapping4));
}

TEST(QueryAnswer, assignments_equal) {
    Assignment mapping1;
    EXPECT_TRUE(mapping1.assign("v1", "1"));
    EXPECT_TRUE(mapping1.assign("v2", "2"));
    Assignment mapping2;
    EXPECT_TRUE(mapping2.assign("v1", "1"));
    EXPECT_TRUE(mapping2.assign("v2", "2"));
    Assignment mapping3;
    EXPECT_TRUE(mapping3.assign("v1", "1"));
    EXPECT_TRUE(mapping3.assign("v2", "1"));
    Assignment mapping4;
    EXPECT_TRUE(mapping4.assign("v1", "2"));
    EXPECT_TRUE(mapping4.assign("v2", "2"));
    Assignment mapping5;
    EXPECT_TRUE(mapping5.assign("v3", "1"));
    EXPECT_TRUE(mapping5.assign("v2", "2"));
    Assignment mapping6;
    EXPECT_TRUE(mapping6.assign("v1", "1"));
    EXPECT_TRUE(mapping6.assign("v3", "2"));
    Assignment mapping7;
    EXPECT_TRUE(mapping7.assign("v1", "1"));
    EXPECT_TRUE(mapping7.assign("v2", "2"));
    EXPECT_TRUE(mapping7.assign("v3", "3"));
    Assignment mapping8;
    EXPECT_TRUE(mapping8.assign("v1", "1"));

    EXPECT_TRUE(mapping1 == mapping1);
    EXPECT_TRUE(mapping2 == mapping2);
    EXPECT_TRUE(mapping3 == mapping3);
    EXPECT_TRUE(mapping4 == mapping4);
    EXPECT_TRUE(mapping5 == mapping5);
    EXPECT_TRUE(mapping6 == mapping6);
    EXPECT_TRUE(mapping7 == mapping7);
    EXPECT_TRUE(mapping8 == mapping8);

    EXPECT_TRUE(mapping1 == mapping2);
    EXPECT_FALSE(mapping1 == mapping3);
    EXPECT_FALSE(mapping1 == mapping4);
    EXPECT_FALSE(mapping1 == mapping5);
    EXPECT_FALSE(mapping1 == mapping6);
    EXPECT_FALSE(mapping1 == mapping7);
    EXPECT_FALSE(mapping1 == mapping8);

    EXPECT_TRUE(mapping2 == mapping1);
    EXPECT_FALSE(mapping2 == mapping3);
    EXPECT_FALSE(mapping2 == mapping4);
    EXPECT_FALSE(mapping2 == mapping5);
    EXPECT_FALSE(mapping2 == mapping6);
    EXPECT_FALSE(mapping2 == mapping7);
    EXPECT_FALSE(mapping2 == mapping8);

    EXPECT_FALSE(mapping3 == mapping4);
    EXPECT_FALSE(mapping3 == mapping5);
    EXPECT_FALSE(mapping3 == mapping6);
    EXPECT_FALSE(mapping3 == mapping7);
    EXPECT_FALSE(mapping3 == mapping8);

    EXPECT_FALSE(mapping4 == mapping5);
    EXPECT_FALSE(mapping4 == mapping6);
    EXPECT_FALSE(mapping4 == mapping7);
    EXPECT_FALSE(mapping4 == mapping8);

    EXPECT_FALSE(mapping5 == mapping6);
    EXPECT_FALSE(mapping5 == mapping7);
    EXPECT_FALSE(mapping5 == mapping8);

    EXPECT_FALSE(mapping6 == mapping7);
    EXPECT_FALSE(mapping6 == mapping8);

    EXPECT_FALSE(mapping7 == mapping8);

    unordered_set<Assignment> set;
    set.insert(mapping1);
    EXPECT_TRUE(set.find(mapping1) != set.end());
    EXPECT_TRUE(set.find(mapping2) != set.end());
    EXPECT_TRUE(set.find(mapping3) == set.end());
    EXPECT_TRUE(set.find(mapping4) == set.end());
    EXPECT_TRUE(set.find(mapping5) == set.end());
    EXPECT_TRUE(set.find(mapping6) == set.end());
    EXPECT_TRUE(set.find(mapping7) == set.end());
    EXPECT_TRUE(set.find(mapping8) == set.end());
    set.insert(mapping2);
    EXPECT_TRUE(set.find(mapping1) != set.end());
    EXPECT_TRUE(set.find(mapping2) != set.end());
    EXPECT_TRUE(set.find(mapping3) == set.end());
    EXPECT_TRUE(set.find(mapping4) == set.end());
    EXPECT_TRUE(set.find(mapping5) == set.end());
    EXPECT_TRUE(set.find(mapping6) == set.end());
    EXPECT_TRUE(set.find(mapping7) == set.end());
    EXPECT_TRUE(set.find(mapping8) == set.end());
    set.insert(mapping3);
    EXPECT_TRUE(set.find(mapping1) != set.end());
    EXPECT_TRUE(set.find(mapping2) != set.end());
    EXPECT_TRUE(set.find(mapping3) != set.end());
    EXPECT_TRUE(set.find(mapping4) == set.end());
    EXPECT_TRUE(set.find(mapping5) == set.end());
    EXPECT_TRUE(set.find(mapping6) == set.end());
    EXPECT_TRUE(set.find(mapping7) == set.end());
    EXPECT_TRUE(set.find(mapping8) == set.end());
}

TEST(QueryAnswer, handles_answer_basics) {
    // Tests add_handle()
    QueryAnswer query_answer1("h1", 0);
    query_answer1.assignment.assign("v1", "1");
    EXPECT_EQ(query_answer1.handles.size(), 1);
    EXPECT_TRUE(set<string>(query_answer1.handles.begin(), query_answer1.handles.end()) ==
                set<string>({"h1"}));
    query_answer1.add_handle("hx");
    EXPECT_EQ(query_answer1.handles.size(), 2);
    EXPECT_TRUE(set<string>(query_answer1.handles.begin(), query_answer1.handles.end()) ==
                set<string>({"h1", "hx"}));

    // Tests merge()
    QueryAnswer query_answer2("h2", 0);
    query_answer2.assignment.assign("v2", "2");
    query_answer2.add_handle("hx");
    query_answer2.merge(&query_answer1);
    EXPECT_EQ(query_answer2.handles.size(), 3);
    EXPECT_TRUE(set<string>(query_answer2.handles.begin(), query_answer2.handles.end()) ==
                set<string>({"h1", "h2", "hx"}));
    EXPECT_FALSE(query_answer2.assignment.assign("v1", "x"));
    EXPECT_FALSE(query_answer2.assignment.assign("v2", "x"));
    EXPECT_TRUE(query_answer2.assignment.assign("v3", "x"));

    // Tests copy()
    QueryAnswer* query_answer3 = QueryAnswer::copy(&query_answer2);
    EXPECT_EQ(query_answer3->handles.size(), 3);
    EXPECT_TRUE(set<string>(query_answer3->handles.begin(), query_answer3->handles.end()) ==
                set<string>({"h1", "h2", "hx"}));
    EXPECT_FALSE(query_answer3->assignment.assign("v1", "x"));
    EXPECT_FALSE(query_answer3->assignment.assign("v2", "x"));
    EXPECT_FALSE(query_answer3->assignment.assign("v3", "y"));
    EXPECT_TRUE(query_answer3->assignment.assign("v4", "x"));
}

void query_answers_equal(QueryAnswer* qa1, QueryAnswer* qa2) {
    EXPECT_TRUE(double_equals(qa1->strength, qa2->strength));
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
            input.assignment.assign(strdup(sequential_label(label_count).c_str()),
                                    strdup(random_handle().c_str()));
        }

        query_answers_equal(&input, QueryAnswer::copy(&input));
        string token_string = input.tokenize();
        QueryAnswer output(0.0);
        output.untokenize(token_string);
        query_answers_equal(&input, &output);
    }
}

TEST(QueryAnswer, get_query_answer_element) {
    QueryAnswer answer;
    answer.handles.push_back("h1");
    answer.handles.push_back("h2");
    answer.assignment.assign("v1", "h3");
    answer.assignment.assign("v2", "h4");
    QueryAnswerElement element1(0);
    QueryAnswerElement element2(1);
    QueryAnswerElement element3("v1");
    QueryAnswerElement element4("v2");
    QueryAnswerElement element5(2);
    QueryAnswerElement element6("v3");
    EXPECT_EQ(answer.get(element1), "h1");
    EXPECT_EQ(answer.get(element2), "h2");
    EXPECT_EQ(answer.get(element3), "h3");
    EXPECT_EQ(answer.get(element4), "h4");
    EXPECT_THROW(answer.get(element5), runtime_error);
    EXPECT_THROW(answer.get(element6), runtime_error);
}

TEST(QueryAnswer, rewrite_query) {
    QueryAnswer answer;
    answer.handles.push_back("h1");
    answer.handles.push_back("h2");
    answer.assignment.assign("v1", "h3");
    answer.assignment.assign("v2", "h4");
    QueryAnswerElement element1("v2");
    QueryAnswerElement element2(1);
    vector<string> original_query = {
        "AND",      "2",  "LINK_TEMPLATE", "Expression", "2", "NODE", "Symbol", "p1",
        "VARIABLE", "v1", "LINK_TEMPLATE", "Expression", "2", "NODE", "Symbol", "p2",
        "VARIABLE", "v2", "LINK_TEMPLATE", "Expression", "2", "NODE", "Symbol", "p3",
        "VARIABLE", "v3",
    };
    vector<string> expected = {
        "AND",      "2",  "LINK_TEMPLATE", "Expression", "2", "NODE", "Symbol", "p1",
        "ATOM",     "h4", "LINK_TEMPLATE", "Expression", "2", "NODE", "Symbol", "p2",
        "VARIABLE", "v2", "LINK_TEMPLATE", "Expression", "2", "NODE", "Symbol", "p3",
        "ATOM",     "h2",
    };
    vector<string> new_query = {"blah"};
    map<string, QueryAnswerElement> replacements = {{"v1", element1}, {"v3", element2}};
    answer.rewrite_query(original_query, replacements, new_query);
    EXPECT_EQ(expected, new_query);
}
