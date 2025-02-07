#include "gtest/gtest.h"

#include "AttentionBrokerServer.h"

using namespace attention_broker_server;

class TestRequestQueue: public RequestQueue {
    public:
        TestRequestQueue(unsigned int n) : RequestQueue(n) {
        }
        unsigned int test_current_size() {
            return current_size();
        }
        unsigned int test_current_start() {
            return current_start();
        }
        unsigned int test_current_end() {
            return current_end();
        }
};

class TestMessage {
    public:
        int message;
        TestMessage(int n) {
            message = n;
        }
};

TEST(RequestQueueTest, basics) {
    
    dasproto::Empty empty;
    dasproto::Ack ack;

    TestRequestQueue q1((unsigned int) 5);
    EXPECT_TRUE(q1.test_current_size() == 5);
    q1.enqueue((void *) "1");
    EXPECT_EQ((char *) q1.dequeue(), "1");
    q1.enqueue((void *) "2");
    q1.enqueue((void *) "3");
    q1.enqueue((void *) "4");
    q1.enqueue((void *) "5");
    EXPECT_TRUE(q1.test_current_size() == 5);
    EXPECT_EQ((char *) q1.dequeue(), "2");
    q1.enqueue((void *) "6");
    q1.enqueue((void *) "7");
    EXPECT_TRUE(q1.test_current_size() == 5);
    EXPECT_EQ((char *) q1.dequeue(), "3");
    q1.enqueue((void *) "8");
    EXPECT_EQ((char *) q1.dequeue(), "4");
    q1.enqueue((void *) "9");
    EXPECT_EQ((char *) q1.dequeue(), "5");
    q1.enqueue((void *) "10");
    EXPECT_TRUE(q1.test_current_size() == 5);
    EXPECT_EQ((char *) q1.dequeue(), "6");
    EXPECT_EQ((char *) q1.dequeue(), "7");
    q1.enqueue((void *) "11");
    q1.enqueue((void *) "12");
    EXPECT_TRUE(q1.test_current_size() == 5);
    EXPECT_TRUE(q1.test_current_start() == 2);
    EXPECT_TRUE(q1.test_current_end() == 2);
    q1.enqueue((void *) "13");
    EXPECT_TRUE(q1.test_current_size() == 10);
    EXPECT_TRUE(q1.test_current_start() == 0);
    EXPECT_TRUE(q1.test_current_end() == 6);
    q1.enqueue((void *) "14");
    EXPECT_EQ((char *) q1.dequeue(), "8");
    EXPECT_EQ((char *) q1.dequeue(), "9");
    EXPECT_EQ((char *) q1.dequeue(), "10");
    EXPECT_EQ((char *) q1.dequeue(), "11");
    EXPECT_EQ((char *) q1.dequeue(), "12");
    EXPECT_EQ((char *) q1.dequeue(), "13");
    EXPECT_EQ((char *) q1.dequeue(), "14");
}
