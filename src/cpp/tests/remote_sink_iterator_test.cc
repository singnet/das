#include <cstdlib>
#include "gtest/gtest.h"

#include "RemoteSink.h"
#include "Source.h"
#include "RemoteIterator.h"
#include "AtomDBSingleton.h"
#include "test_utils.h"

using namespace query_engine;
using namespace query_element;
using namespace query_node;

class TestSource : public Source {
    public:
    TestSource(const string &id) {
        this->id = id;
    }
    void add(QueryAnswer *qa) {
        this->output_buffer->add_query_answer(qa);
        Utils::sleep(1000);
    }
    void finished() {
        this->output_buffer->query_answers_finished();
        Utils::sleep(1000);
    }
};

TEST(RemoteSinkIterator, basics) {
    string consumer_id = "localhost:30700";
    string producer_id = "localhost:30701";

    string input_element_id = "test_source";
    TestSource input(input_element_id);
    RemoteIterator consumer(consumer_id);
    RemoteSink producer(&input, producer_id, consumer_id);
    Utils::sleep(1000);

    EXPECT_FALSE(consumer.finished());

    QueryAnswer *qa;
    QueryAnswer qa0("h0", 0.0);
    QueryAnswer qa1("h1", 0.1);
    QueryAnswer qa2("h2", 0.2);

    input.add(&qa0);
    input.add(&qa1);

    EXPECT_FALSE(consumer.finished());
    EXPECT_FALSE((qa = consumer.pop()) == NULL);
    EXPECT_TRUE(strcmp(qa->handles[0], "h0") == 0);
    EXPECT_TRUE(double_equals(qa->importance, 0.0));

    EXPECT_FALSE(consumer.finished());
    EXPECT_FALSE((qa = consumer.pop()) == NULL);
    EXPECT_TRUE(strcmp(qa->handles[0], "h1") == 0);
    EXPECT_TRUE(double_equals(qa->importance, 0.1));

    EXPECT_TRUE((qa = consumer.pop()) == NULL);
    EXPECT_FALSE(consumer.finished());

    input.add(&qa2);
    input.finished();
    EXPECT_FALSE(consumer.finished());

    EXPECT_FALSE(consumer.finished());
    EXPECT_FALSE((qa = consumer.pop()) == NULL);
    EXPECT_TRUE(strcmp(qa->handles[0], "h2") == 0);
    EXPECT_TRUE(double_equals(qa->importance, 0.2));
    Utils::sleep(5000); // XXXXXXXXXXXXXXXXXXXXXXXXXXX
    EXPECT_TRUE(consumer.finished());
}
