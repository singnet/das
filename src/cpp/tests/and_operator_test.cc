#include <cstdlib>
#include <cstring>
#include "gtest/gtest.h"

#include "Source.h"
#include "Sink.h"
#include "QueryAnswer.h"
#include "And.h"
#include "test_utils.h"

using namespace query_engine;
using namespace query_element;

#define SLEEP_DURATION ((unsigned int) 1000)

class TestSource : public Source {

    public:

        TestSource(unsigned int count) {
            this->id = "TestSource_" + to_string(count);
        }

        ~TestSource() {
        }

        void add(
            const char *handle,
            double importance,
            const array<const char *, 1> &labels,
            const array<const char *, 1> &values,
            bool sleep_flag = true) {

            QueryAnswer *query_answer = new QueryAnswer(handle, importance);
            for (unsigned int i = 0; i < labels.size(); i++) {
                query_answer->assignment.assign(labels[i], values[i]);
            }
            this->output_buffer->add_query_answer(query_answer);
            if (sleep_flag) {
                Utils::sleep(SLEEP_DURATION);
            }
        }

        void query_answers_finished() {
            return this->output_buffer->query_answers_finished();
        }
};

class TestSink : public Sink {
    public:
        TestSink(QueryElement *precedent) : 
            Sink(precedent, "TestSink(" + precedent->id + ")") { 
        } 
        ~TestSink() {
        }
        bool empty() { return this->input_buffer->is_query_answers_empty(); }
        bool finished() { return this->input_buffer->is_query_answers_finished(); }
        QueryAnswer *pop() { return this->input_buffer->pop_query_answer(); }
};

void check_query_answer(
    string tag,
    QueryAnswer *query_answer,
    double importance,
    unsigned int handles_size,
    const array<const char *, 2> &handles) {

    cout << "check_query_answer(" + tag + ")" << endl;
    EXPECT_TRUE(double_equals(query_answer->importance, importance));
    EXPECT_EQ(query_answer->handles_size, 2);
    for (unsigned int i = 0; i < handles_size; i++) {
        EXPECT_TRUE(strcmp(query_answer->handles[i], handles[i]) == 0);
    }
}

TEST(AndOperator, basics) {
    
    TestSource source1(1);
    TestSource source2(2);
    And<2> and_operator({&source1, &source2});
    TestSink sink(&and_operator);
    QueryAnswer *query_answer;

    EXPECT_TRUE(sink.empty()); EXPECT_FALSE(sink.finished());

    // --------------------------------------------------
    // Expected processing order:
    // h1: 0.5, 0.4, 0.3
    // h2: 0.3, 0.2, 0.1
    // 0 0 - 0.15
    // 1 0 - 0.12 INVALID
    // 0 1 - 0.10
    // 2 0 - 0.09
    // 1 1 - 0.08
    // 2 1 - 0.06
    // 0 2 - 0.05
    // 1 2 - 0.04
    // 2 2 - 0.03
    // --------------------------------------------------

    source1.add("h1_0", 0.5, {"v1_0"}, {"1"});
    source2.add("h2_0", 0.3, {"v1_1"}, {"2"});
    source2.add("h2_1", 0.2, {"v2_1"}, {"1"});
    EXPECT_TRUE(sink.empty()); EXPECT_FALSE(sink.finished());
    source1.add("h1_1", 0.4, {"v1_1"}, {"1"});
    EXPECT_FALSE(sink.empty()); EXPECT_FALSE(sink.finished());
    EXPECT_FALSE((query_answer = sink.pop()) == NULL);
    EXPECT_TRUE(sink.empty()); EXPECT_FALSE(sink.finished());
    check_query_answer("1", query_answer, 0.5, 2, {"h1_0", "h2_0"});
    EXPECT_TRUE(strcmp(query_answer->assignment.get("v1_0"), "1") == 0);
    EXPECT_TRUE(strcmp(query_answer->assignment.get("v1_1"), "2") == 0);
    source1.add("h1_2", 0.3, {"v1_2"}, {"1"});
    EXPECT_TRUE(sink.empty()); EXPECT_FALSE(sink.finished());
    source2.add("h2_2", 0.1, {"v2_2"}, {"1"});
    EXPECT_TRUE(sink.empty()); EXPECT_FALSE(sink.finished());
    source1.query_answers_finished();
    EXPECT_TRUE(sink.empty()); EXPECT_FALSE(sink.finished());
    source2.query_answers_finished();
    Utils::sleep(SLEEP_DURATION);
    EXPECT_FALSE(sink.empty()); EXPECT_TRUE(sink.finished());

    // {"h1_1", "h2_0"} is not popped because it's invalid

    EXPECT_FALSE((query_answer = sink.pop()) == NULL);
    check_query_answer("3", query_answer, 0.5, 2, {"h1_0", "h2_1"});

    EXPECT_FALSE((query_answer = sink.pop()) == NULL);
    check_query_answer("4", query_answer, 0.3, 2, {"h1_2", "h2_0"});

    EXPECT_FALSE((query_answer = sink.pop()) == NULL);
    check_query_answer("5", query_answer, 0.4, 2, {"h1_1", "h2_1"});

    EXPECT_FALSE((query_answer = sink.pop()) == NULL);
    check_query_answer("6", query_answer, 0.3, 2, {"h1_2", "h2_1"});

    EXPECT_FALSE((query_answer = sink.pop()) == NULL);
    check_query_answer("7", query_answer, 0.5, 2, {"h1_0", "h2_2"});

    EXPECT_FALSE((query_answer = sink.pop()) == NULL);
    check_query_answer("8", query_answer, 0.4, 2, {"h1_1", "h2_2"});

    EXPECT_FALSE((query_answer = sink.pop()) == NULL);
    EXPECT_TRUE(sink.empty());
    check_query_answer("9", query_answer, 0.3, 2, {"h1_2", "h2_2"});
    Utils::sleep(SLEEP_DURATION);

    EXPECT_TRUE(sink.empty()); EXPECT_TRUE(sink.finished());
}

TEST(AndOperator, operation_logic) {
    
    class ImportanceFitnessPair {
        public:
        double importance;
        double fitness;
        ImportanceFitnessPair() {}
        ImportanceFitnessPair(const ImportanceFitnessPair &other) {
            this->importance = other.importance;
            this->fitness = other.fitness;
        }
        ImportanceFitnessPair& operator=(const ImportanceFitnessPair &other) {
            this->importance = other.importance;
            this->fitness = other.fitness;
            return *this;
        }
        bool operator<(const ImportanceFitnessPair &other) const {
            return this->fitness < other.fitness;
        }
        bool operator>(const ImportanceFitnessPair &other) const {
            return this->fitness > other.fitness;
        }
    };

    cout << "SETUP" << endl;

    unsigned int clause_count = 3;
    unsigned int link_count = 100;
    array<array<double, 100>, 3> importance;
    priority_queue<ImportanceFitnessPair> fitness_heap;
    ImportanceFitnessPair pair;
    QueryAnswer *query_answer;
    TestSource *source[3];
    for (unsigned int clause = 0; clause < clause_count; clause++) {
        source[clause] = new TestSource(clause);
    }
    And<3> *and_operator = new And<3>((QueryElement **) source);
    TestSink *sink = new TestSink(and_operator);

    for (unsigned int clause = 0; clause < clause_count; clause++) {
        for (unsigned int link = 0; link < link_count; link++) {
            importance[clause][link] = random_importance();
        }
        std::sort(
            std::begin(importance[clause]), 
            std::end(importance[clause]), 
            std::greater<double>{});
    }

    cout << "QUEUES POPULATION" << endl;

    for (unsigned int clause = 0; clause < clause_count; clause++) {
        for (unsigned int link = 0; link < link_count; link++) {
            source[clause]->add(
                random_handle().c_str(),
                importance[clause][link],
                {"v"},
                {"1"},
                false);
        }
        source[clause]->query_answers_finished();
    }

    cout << "MATRIX POPULATION" << endl;

    for (unsigned int i = 0; i < link_count; i++) {
        for (unsigned int j = 0; j < link_count; j++) {
            for (unsigned int k = 0; k < link_count; k++) {
                pair.importance = importance[0][i] > importance[1][j] ? importance[0][i] : importance[1][j];
                pair.importance = importance[2][k] > pair.importance ? importance[2][k] : pair.importance;
                pair.fitness = importance[0][i] * importance[1][j] * importance[2][k];
                fitness_heap.push(pair);
            }
        }
    }

    Utils::sleep(5000);
    cout << "TEST CHECKS" << endl;

    unsigned int count = 0;
    while (! (sink->empty() && sink->finished())) {
        if (sink->empty()) {
            Utils::sleep();
            continue;
        }
        EXPECT_FALSE((query_answer = sink->pop()) == NULL);
        pair = fitness_heap.top();
        cout << count << " CHECK: " << query_answer->importance << " " << pair.importance << " (" << pair.fitness << ")" << endl;
        EXPECT_TRUE(double_equals(query_answer->importance, pair.importance));
        fitness_heap.pop();
        count++;
    }

    Utils::sleep(5000);
    cout << "TEAR DOWN" << endl;

    delete sink;
    delete and_operator;
    for (unsigned int clause = 0; clause < clause_count; clause++) {
        delete source[clause];
    }
}
