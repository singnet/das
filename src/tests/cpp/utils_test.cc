#define LOG_LEVEL DEBUG_LEVEL
#include "Utils.h"

#include <gtest/gtest.h>

#include <fstream>
#include <iostream>
#include <vector>

#include "test_utils.h"

using namespace std;
using namespace commons;

void f1() {
    STACK_TRACE();
    RAISE_ERROR("Error in f1()");
    return;
}

void f2() {
    STACK_TRACE();
    f1();
    return;
}

void f3() {
    STACK_TRACE();
    f2();
    return;
}

TEST(LocalFileTestSuite, read_and_split) {
    string fname = "UtilsTest_read_and_aplit.txt";
    string test_file = "c1 1 2 3\nc2 1 2  4\nc3 \nc4\n c5 1 2\n \n\nc6 1 2 34\nc7,111,22,3\n\n";
    make_tmp_file(fname, test_file);
    fname = "/tmp/" + fname;
    ifstream file(fname);
    EXPECT_TRUE(file.is_open());
    vector<string> line;
    EXPECT_TRUE(Utils::read_and_split(line, file));
    EXPECT_EQ(line, vector<string>({"c1", "1", "2", "3"}));
    line.clear();
    EXPECT_TRUE(Utils::read_and_split(line, file));
    EXPECT_EQ(line, vector<string>({"c2", "1", "2", "", "4"}));
    line.clear();
    EXPECT_TRUE(Utils::read_and_split(line, file));
    EXPECT_EQ(line, vector<string>({"c3"}));
    line.clear();
    EXPECT_TRUE(Utils::read_and_split(line, file));
    EXPECT_EQ(line, vector<string>({"c4"}));
    line.clear();
    EXPECT_TRUE(Utils::read_and_split(line, file));
    EXPECT_EQ(line, vector<string>({"", "c5", "1", "2"}));
    line.clear();
    EXPECT_TRUE(Utils::read_and_split(line, file));
    EXPECT_EQ(line, vector<string>({""}));
    line.clear();
    EXPECT_TRUE(Utils::read_and_split(line, file));
    EXPECT_EQ(line, vector<string>({}));
    line.clear();
    EXPECT_TRUE(Utils::read_and_split(line, file));
    EXPECT_EQ(line, vector<string>({"c6", "1", "2", "34"}));
    line.clear();
    EXPECT_TRUE(Utils::read_and_split(line, file, ','));
    EXPECT_EQ(line, vector<string>({"c7", "111", "22", "3"}));
    line.clear();
    EXPECT_TRUE(Utils::read_and_split(line, file));
    EXPECT_EQ(line, vector<string>({}));
    line.clear();
    line.push_back("blah");
    EXPECT_FALSE(Utils::read_and_split(line, file));
    EXPECT_EQ(line, vector<string>({"blah"}));
}

TEST(LocalFileTestSuite, stack_trace) {
    STACK_TRACE();
    EXPECT_THROW(RAISE_ERROR("Error on toplevel"), runtime_error);
    EXPECT_THROW(f3(), runtime_error);
    EXPECT_THROW(f2(), runtime_error);
    EXPECT_THROW(f1(), runtime_error);
    // FAIL();
}

TEST(LocalFileTestSuite, flip_coin) {
    unsigned int N = 20000;
    float tolerance = 0.07;
    for (float prob : {0.20, 0.50, 0.80}) {
        unsigned int count_true = 0;
        unsigned int count_false = 0;
        unsigned int expected_true = lround(prob * N);
        unsigned int expected_false = N - expected_true;
        unsigned int delta_true = lround(tolerance * expected_true);
        unsigned int delta_false = lround(tolerance * expected_false);
        for (unsigned int i = 0; i < N; i++) {
            if (Utils::flip_coin(prob)) {
                count_true++;
            } else {
                count_false++;
            }
        }
        EXPECT_TRUE(count_true >= (expected_true - delta_true));
        EXPECT_TRUE(count_true <= (expected_true + delta_true));
        EXPECT_TRUE(count_false >= (expected_false - delta_false));
        EXPECT_TRUE(count_false <= (expected_false + delta_false));
    }
    for (unsigned int i = 0; i < N; i++) {
        EXPECT_TRUE(Utils::flip_coin(1.0));
        EXPECT_FALSE(Utils::flip_coin(0.0));
    }
    EXPECT_THROW(Utils::flip_coin(1.1), runtime_error);
    EXPECT_THROW(Utils::flip_coin(-0.5), runtime_error);
}

TEST(LocalFileTestSuite, uint_rand) {
    for (pair<unsigned int, unsigned int> p : vector<pair<unsigned int, unsigned int>>(
             {{0, 1}, {0, 2}, {0, 3}, {2, 3}, {2, 4}, {2, 5}, {105, 1200}})) {
        for (unsigned int i = 0; i < 10000; i++) {
            unsigned int closed_lower = p.first;
            unsigned int open_upper = p.second;
            unsigned int number = Utils::uint_rand(closed_lower, open_upper);
            EXPECT_TRUE(number >= closed_lower);
            EXPECT_TRUE(number < open_upper);
        }
    }
    EXPECT_THROW(Utils::uint_rand(0, 0), runtime_error);
    EXPECT_THROW(Utils::uint_rand(2, 2), runtime_error);
    EXPECT_THROW(Utils::uint_rand(2, 1), runtime_error);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    Utils::init_random(0);
    return RUN_ALL_TESTS();
}
