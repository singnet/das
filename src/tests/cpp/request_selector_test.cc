#include "RequestSelector.h"
#include "SharedQueue.h"
#include "Utils.h"
#include "gtest/gtest.h"

using namespace attention_broker_server;

class TestMessage {
public:
  int message;
  TestMessage(int n) { message = n; }
};

TEST(RequestSelectorTest, even_thread_count) {
  SharedQueue *stimulus = new SharedQueue(1);
  SharedQueue *correlation = new SharedQueue(1);

  RequestSelector *selector0 = RequestSelector::factory(
      SelectorType::EVEN_THREAD_COUNT, 0, stimulus, correlation);
  RequestSelector *selector1 = RequestSelector::factory(
      SelectorType::EVEN_THREAD_COUNT, 1, stimulus, correlation);

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
