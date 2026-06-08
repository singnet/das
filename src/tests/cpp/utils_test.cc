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

TEST(UtilsTest, read_and_split) {
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

TEST(UtilsTest, stack_trace) {
    STACK_TRACE();
    EXPECT_THROW(RAISE_ERROR("Error on toplevel"), runtime_error);
    EXPECT_THROW(f3(), runtime_error);
    EXPECT_THROW(f2(), runtime_error);
    EXPECT_THROW(f1(), runtime_error);
    // FAIL();
}
