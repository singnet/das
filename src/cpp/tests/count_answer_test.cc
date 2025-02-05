#include <cstdlib>
#include <cstring>

#include "CountAnswer.h"
#include "gtest/gtest.h"

using namespace query_engine;

TEST(CountAnswer, count_answer_basics) {
    CountAnswer cout_answer1;
    EXPECT_EQ(cout_answer1.get_count(), UNDEFINED_COUNT);

    CountAnswer cout_answer2(1);
    EXPECT_EQ(cout_answer2.get_count(), 1);
    EXPECT_EQ(cout_answer2.to_string(), "CountAnswer<1>");
    EXPECT_EQ(cout_answer2.tokenize(), "1");

    CountAnswer cout_answer3;
    cout_answer3.untokenize("2");
    EXPECT_EQ(cout_answer3.get_count(), 2);
    EXPECT_EQ(cout_answer3.to_string(), "CountAnswer<2>");
}
