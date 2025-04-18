#include <cstdlib>
#include <cstring>

#include "QueryAnswer.h"
#include "Sink.h"
#include "Source.h"
#include "UniqueAssignmentFilter.h"
#include "gtest/gtest.h"
#include "test_utils.h"

using namespace query_engine;
using namespace query_element;

#define SLEEP_DURATION ((unsigned int) 1000)

class TestSource : public Source {
   public:
    TestSource(unsigned int count) { this->id = "TestSource_" + to_string(count); }

    ~TestSource() {}

    void add(const char* handle,
             double importance,
             const array<const char*, 1>& labels,
             const array<const char*, 1>& values,
             bool sleep_flag = true) {
        QueryAnswer* query_answer = new QueryAnswer(handle, importance);
        for (unsigned int i = 0; i < labels.size(); i++) {
            query_answer->assignment.assign(labels[i], values[i]);
        }
        this->output_buffer->add_query_answer(query_answer);
        if (sleep_flag) {
            Utils::sleep(SLEEP_DURATION);
        }
    }

    void query_answers_finished() { return this->output_buffer->query_answers_finished(); }
};

class TestSink : public Sink {
   public:
    TestSink(shared_ptr<QueryElement> precedent) : Sink(precedent, "TestSink(" + precedent->id + ")") {}
    ~TestSink() {}
    bool empty() { return this->input_buffer->is_query_answers_empty(); }
    bool finished() { return this->input_buffer->is_query_answers_finished(); }
    QueryAnswer* pop() { return this->input_buffer->pop_query_answer(); }
};

void check_query_answer(string tag,
                        QueryAnswer* query_answer,
                        double importance,
                        unsigned int handles_size,
                        const array<const char*, 2>& handles) {
    cout << "check_query_answer(" + tag + ")" << endl;
    EXPECT_TRUE(double_equals(query_answer->importance, importance));
    EXPECT_EQ(query_answer->handles_size, 1);
    for (unsigned int i = 0; i < handles_size; i++) {
        EXPECT_TRUE(strcmp(query_answer->handles[i], handles[i]) == 0);
    }
}

TEST(UniqueAssignmentFilter, basics) {
    auto source = make_shared<TestSource>(10);
    auto filter = make_shared<UniqueAssignmentFilter>(source);
    TestSink sink(filter);
    QueryAnswer* query_answer;

    EXPECT_TRUE(sink.empty());
    EXPECT_FALSE(sink.finished());

    source->add("h1", 0.0, {"v1"}, {"1"});
    source->add("h2", 0.0, {"v2"}, {"2"});
    source->add("h3", 0.0, {"v3"}, {"3"});
    source->add("h4", 0.0, {"v1"}, {"1"});  // Duplicated
    source->add("h2", 0.0, {"v3"}, {"3"});  // Duplicated
    source->add("h3", 0.0, {"v3"}, {"3"});  // Duplicated
    source->add("h1", 0.0, {"v1"}, {"2"});
    source->add("h1", 0.0, {"v2"}, {"1"});
    source->add("h2", 0.0, {"v1"}, {"2"});  // Duplicated
    source->add("h2", 0.0, {"v2"}, {"1"});  // Duplicated
    source->add("h1", 0.0, {"v2"}, {"2"});  // Duplicated
    source->add("h1", 0.0, {"v3"}, {"3"});  // Duplicated

    EXPECT_FALSE(sink.empty());
    EXPECT_FALSE(sink.finished());

    EXPECT_FALSE((query_answer = dynamic_cast<QueryAnswer*>(sink.pop())) == NULL);
    check_query_answer("1", query_answer, 0.0, 1, {"h1"});
    EXPECT_FALSE((query_answer = dynamic_cast<QueryAnswer*>(sink.pop())) == NULL);
    check_query_answer("2", query_answer, 0.0, 1, {"h2"});
    EXPECT_FALSE((query_answer = dynamic_cast<QueryAnswer*>(sink.pop())) == NULL);
    check_query_answer("3", query_answer, 0.0, 1, {"h3"});
    EXPECT_FALSE((query_answer = dynamic_cast<QueryAnswer*>(sink.pop())) == NULL);
    check_query_answer("4", query_answer, 0.0, 1, {"h1"});
    EXPECT_FALSE((query_answer = dynamic_cast<QueryAnswer*>(sink.pop())) == NULL);
    check_query_answer("5", query_answer, 0.0, 1, {"h1"});

    source->add("h4", 0.0, {"v1"}, {"4"});
    EXPECT_FALSE((query_answer = dynamic_cast<QueryAnswer*>(sink.pop())) == NULL);
    check_query_answer("6", query_answer, 0.0, 1, {"h4"});

    EXPECT_TRUE(sink.empty());
    EXPECT_FALSE(sink.finished());

    source->query_answers_finished();
    Utils::sleep(SLEEP_DURATION);

    EXPECT_TRUE(sink.empty());
    EXPECT_TRUE(sink.finished());
}
