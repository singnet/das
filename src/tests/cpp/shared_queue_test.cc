#include "AttentionBrokerServer.h"
#include "gtest/gtest.h"

using namespace attention_broker_server;

class TestSharedQueue : public SharedQueue {
   public:
    TestSharedQueue(unsigned int n) : SharedQueue(n) {}
    unsigned int test_current_size() { return current_size(); }
    unsigned int test_current_start() { return current_start(); }
    unsigned int test_current_end() { return current_end(); }
};

class TestMessage {
   public:
    int message;
    TestMessage(int n) { message = n; }
};

TEST(SharedQueueTest, basics) {
    dasproto::Empty empty;
    dasproto::Ack ack;

    TestSharedQueue q1((unsigned int) 5);
    EXPECT_TRUE(q1.size() == 0);
    EXPECT_TRUE(q1.test_current_size() == 5);
    q1.enqueue((void*) "1");
    EXPECT_TRUE(q1.size() == 1);
    EXPECT_EQ((char*) q1.dequeue(), "1");
    EXPECT_TRUE(q1.size() == 0);
    q1.enqueue((void*) "2");
    q1.enqueue((void*) "3");
    q1.enqueue((void*) "4");
    q1.enqueue((void*) "5");
    EXPECT_TRUE(q1.size() == 4);
    EXPECT_TRUE(q1.test_current_size() == 5);
    EXPECT_EQ((char*) q1.dequeue(), "2");
    q1.enqueue((void*) "6");
    q1.enqueue((void*) "7");
    EXPECT_TRUE(q1.size() == 5);
    EXPECT_TRUE(q1.test_current_size() == 5);
    EXPECT_EQ((char*) q1.dequeue(), "3");
    q1.enqueue((void*) "8");
    EXPECT_EQ((char*) q1.dequeue(), "4");
    q1.enqueue((void*) "9");
    EXPECT_EQ((char*) q1.dequeue(), "5");
    q1.enqueue((void*) "10");
    EXPECT_TRUE(q1.size() == 5);
    EXPECT_TRUE(q1.test_current_size() == 5);
    EXPECT_EQ((char*) q1.dequeue(), "6");
    EXPECT_EQ((char*) q1.dequeue(), "7");
    q1.enqueue((void*) "11");
    q1.enqueue((void*) "12");
    EXPECT_TRUE(q1.size() == 5);
    EXPECT_TRUE(q1.test_current_size() == 5);
    EXPECT_TRUE(q1.test_current_start() == 2);
    EXPECT_TRUE(q1.test_current_end() == 2);
    q1.enqueue((void*) "13");
    EXPECT_TRUE(q1.size() == 6);
    EXPECT_TRUE(q1.test_current_size() == 10);
    EXPECT_TRUE(q1.test_current_start() == 0);
    EXPECT_TRUE(q1.test_current_end() == 6);
    q1.enqueue((void*) "14");
    EXPECT_TRUE(q1.size() == 7);
    EXPECT_EQ((char*) q1.dequeue(), "8");
    EXPECT_EQ((char*) q1.dequeue(), "9");
    EXPECT_EQ((char*) q1.dequeue(), "10");
    EXPECT_EQ((char*) q1.dequeue(), "11");
    EXPECT_EQ((char*) q1.dequeue(), "12");
    EXPECT_EQ((char*) q1.dequeue(), "13");
    EXPECT_TRUE(q1.size() == 1);
    EXPECT_EQ((char*) q1.dequeue(), "14");
    EXPECT_TRUE(q1.size() == 0);

    SharedQueue q2;
    unsigned long p1 = 10107;
    q2.enqueue((void*) p1);
    unsigned long p2 = (unsigned long) q2.dequeue();
    EXPECT_EQ(p1, p2);
}
