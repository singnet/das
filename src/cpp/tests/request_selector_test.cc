#include "gtest/gtest.h"

#include "Utils.h"
#include "RequestQueue.h"
#include "RequestSelector.h"

using namespace attention_broker_server;

class TestMessage {
    public:
        int message;
        TestMessage(int n) {
            message = n;
        }
};

TEST(RequestSelectorTest, even_thread_count) {
    
    RequestQueue *stimulus = new RequestQueue(1);
    RequestQueue *correlation = new RequestQueue(1);

    RequestSelector *selector0 = RequestSelector::factory(
        SelectorType::EVEN_THREAD_COUNT,
        0,
        stimulus,
        correlation);
    RequestSelector *selector1 = RequestSelector::factory(
        SelectorType::EVEN_THREAD_COUNT,
        1,
        stimulus,
        correlation);

    pair<RequestType, void *> request;
    for (int i = 0; i < 1000; i++) {
        if (Utils::flip_coin()) {
            request = selector0->next();
            EXPECT_EQ(request.first, RequestType::STIMULUS);
        } else {
            request = selector1->next();
            EXPECT_EQ(request.first, RequestType::CORRELATION);
        }
    }

    delete stimulus;
    delete correlation;
}
