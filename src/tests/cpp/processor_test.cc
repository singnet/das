#include <gtest/gtest.h>

#include "Processor.h"

using namespace std;
using namespace processor;

TEST(ProcessorTest, basics) {
    Processor p1("blah");
    EXPECT_THROW(Processor p2(""), runtime_error);
    EXPECT_EQ(p1.to_string(), "blah");

    EXPECT_FALSE(p1.is_setup());
    EXPECT_FALSE(p1.is_finished());

    EXPECT_THROW(p1.start(), runtime_error);
    EXPECT_THROW(p1.stop(), runtime_error);
    EXPECT_FALSE(p1.is_setup());
    EXPECT_FALSE(p1.is_finished());
    p1.setup();
    EXPECT_TRUE(p1.is_setup());
    EXPECT_FALSE(p1.is_finished());

    EXPECT_THROW(p1.setup(), runtime_error);
    EXPECT_THROW(p1.stop(), runtime_error);
    EXPECT_TRUE(p1.is_setup());
    EXPECT_FALSE(p1.is_finished());
    p1.start();
    EXPECT_TRUE(p1.is_setup());
    EXPECT_FALSE(p1.is_finished());

    EXPECT_THROW(p1.setup(), runtime_error);
    EXPECT_THROW(p1.start(), runtime_error);
    EXPECT_TRUE(p1.is_setup());
    EXPECT_FALSE(p1.is_finished());
    p1.stop();
    EXPECT_TRUE(p1.is_setup());
    EXPECT_TRUE(p1.is_finished());

    EXPECT_THROW(p1.setup(), runtime_error);
    EXPECT_THROW(p1.start(), runtime_error);
    EXPECT_THROW(p1.stop(), runtime_error);
    EXPECT_TRUE(p1.is_setup());
    EXPECT_TRUE(p1.is_finished());
}
