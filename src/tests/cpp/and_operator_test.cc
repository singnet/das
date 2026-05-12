#include <cstdlib>
#include <cstring>

#include "And.h"
#include "QueryAnswer.h"
#include "Sink.h"
#include "Source.h"
#include "gtest/gtest.h"
#include "test_utils.h"

using namespace query_engine;
using namespace query_element;

#define SLEEP_DURATION ((unsigned int) 1000)

class TestSource : public Source {
   public:
    TestSource(unsigned int count) { this->id = "TestSource_" + std::to_string(count); }
    TestSource() { this->id = "TestSource_" +  Utils::random_string(30); }

    ~TestSource() {}

    void add(const char* handle,
             double importance,
             const vector<string>& labels,
             const vector<string>& values,
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
    TestSink(shared_ptr<QueryElement> precedent) : Sink(precedent, "TestSink(" + precedent->id + "," + Utils::random_string(30) + ")") {}
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
    LOG_INFO("check_query_answer(" + tag + "); " + query_answer->to_string());
    EXPECT_TRUE(double_equals(query_answer->importance, importance));
    EXPECT_EQ(query_answer->get_handles_size(), 2);
    set<string> set_handles;
    for (unsigned int i = 0; i < handles.size(); i++) {
        set_handles.insert(handles[i]);
    }
    EXPECT_TRUE(set<string>(query_answer->get_handles_vector().begin(),
                            query_answer->get_handles_vector().end()) == set_handles);
}

TEST(AndOperator, basics) {
    auto source1 = make_shared<TestSource>();
    auto source2 = make_shared<TestSource>();
    auto and_operator = make_shared<And<2>>(array<shared_ptr<QueryElement>, 2>({source1, source2}));
    TestSink sink(and_operator);
    QueryAnswer* query_answer;

    EXPECT_TRUE(sink.empty());
    EXPECT_FALSE(sink.finished());

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

    source1->add("h1_0", 0.5, {"v1_0"}, {"1"});
    source2->add("h2_0", 0.3, {"v1_1"}, {"2"});
    source2->add("h2_1", 0.2, {"v2_1"}, {"1"});
    EXPECT_TRUE(sink.empty());
    EXPECT_FALSE(sink.finished());
    source1->add("h1_1", 0.4, {"v1_1"}, {"1"});
    EXPECT_FALSE(sink.empty());
    EXPECT_FALSE(sink.finished());
    EXPECT_FALSE((query_answer = dynamic_cast<QueryAnswer*>(sink.pop())) == NULL);
    EXPECT_TRUE(sink.empty());
    EXPECT_FALSE(sink.finished());
    check_query_answer("1", query_answer, 0.5, 2, {"h1_0", "h2_0"});
    EXPECT_TRUE(query_answer->assignment.get("v1_0") == "1");
    EXPECT_TRUE(query_answer->assignment.get("v1_1") == "2");
    source1->add("h1_2", 0.3, {"v1_2"}, {"1"});
    EXPECT_TRUE(sink.empty());
    EXPECT_FALSE(sink.finished());
    source2->add("h2_2", 0.1, {"v2_2"}, {"1"});
    EXPECT_TRUE(sink.empty());
    EXPECT_FALSE(sink.finished());
    source1->query_answers_finished();
    EXPECT_TRUE(sink.empty());
    EXPECT_FALSE(sink.finished());
    source2->query_answers_finished();
    Utils::sleep(SLEEP_DURATION);
    EXPECT_FALSE(sink.empty());
    EXPECT_TRUE(sink.finished());

    // {"h1_1", "h2_0"} is not popped because it's invalid

    EXPECT_FALSE((query_answer = dynamic_cast<QueryAnswer*>(sink.pop())) == NULL);
    check_query_answer("3", query_answer, 0.5, 2, {"h1_0", "h2_1"});

    EXPECT_FALSE((query_answer = dynamic_cast<QueryAnswer*>(sink.pop())) == NULL);
    check_query_answer("4", query_answer, 0.3, 2, {"h1_2", "h2_0"});

    EXPECT_FALSE((query_answer = dynamic_cast<QueryAnswer*>(sink.pop())) == NULL);
    check_query_answer("5", query_answer, 0.4, 2, {"h1_1", "h2_1"});

    EXPECT_FALSE((query_answer = dynamic_cast<QueryAnswer*>(sink.pop())) == NULL);
    check_query_answer("6", query_answer, 0.3, 2, {"h1_2", "h2_1"});

    EXPECT_FALSE((query_answer = dynamic_cast<QueryAnswer*>(sink.pop())) == NULL);
    check_query_answer("7", query_answer, 0.5, 2, {"h1_0", "h2_2"});

    EXPECT_FALSE((query_answer = dynamic_cast<QueryAnswer*>(sink.pop())) == NULL);
    check_query_answer("8", query_answer, 0.4, 2, {"h1_1", "h2_2"});

    EXPECT_FALSE((query_answer = dynamic_cast<QueryAnswer*>(sink.pop())) == NULL);
    EXPECT_TRUE(sink.empty());
    check_query_answer("9", query_answer, 0.3, 2, {"h1_2", "h2_2"});
    Utils::sleep(SLEEP_DURATION);

    EXPECT_TRUE(sink.empty());
    EXPECT_TRUE(sink.finished());
}

TEST(AndOperator, operation_logic) {
    class ImportanceFitnessPair {
       public:
        double importance;
        double fitness;
        ImportanceFitnessPair() {}
        ImportanceFitnessPair(const ImportanceFitnessPair& other) {
            this->importance = other.importance;
            this->fitness = other.fitness;
        }
        ImportanceFitnessPair& operator=(const ImportanceFitnessPair& other) {
            this->importance = other.importance;
            this->fitness = other.fitness;
            return *this;
        }
        bool operator<(const ImportanceFitnessPair& other) const {
            return this->fitness < other.fitness;
        }
        bool operator>(const ImportanceFitnessPair& other) const {
            return this->fitness > other.fitness;
        }
    };

    cout << "SETUP" << endl;

    unsigned int clause_count = 3;
    unsigned int link_count = 100;
    array<array<double, 100>, 3> importance;
    priority_queue<ImportanceFitnessPair> fitness_heap;
    ImportanceFitnessPair pair;
    QueryAnswer* query_answer;
    array<shared_ptr<QueryElement>, 3> source;
    for (unsigned int clause = 0; clause < clause_count; clause++) {
        source[clause] = make_shared<TestSource>(clause);
    }
    auto and_operator = make_shared<And<3>>(source);
    auto sink = make_shared<TestSink>(and_operator);

    for (unsigned int clause = 0; clause < clause_count; clause++) {
        for (unsigned int link = 0; link < link_count; link++) {
            importance[clause][link] = random_importance();
        }
        std::sort(std::begin(importance[clause]), std::end(importance[clause]), std::greater<double>{});
    }

    cout << "QUEUES POPULATION" << endl;

    for (unsigned int clause = 0; clause < clause_count; clause++) {
        auto test_source = dynamic_pointer_cast<TestSource>(source[clause]);
        for (unsigned int link = 0; link < link_count; link++) {
            test_source->add(random_handle().c_str(), importance[clause][link], {"v"}, {"1"}, false);
        }
        test_source->query_answers_finished();
    }

    cout << "MATRIX POPULATION" << endl;

    for (unsigned int i = 0; i < link_count; i++) {
        for (unsigned int j = 0; j < link_count; j++) {
            for (unsigned int k = 0; k < link_count; k++) {
                pair.importance =
                    importance[0][i] > importance[1][j] ? importance[0][i] : importance[1][j];
                pair.importance =
                    importance[2][k] > pair.importance ? importance[2][k] : pair.importance;
                pair.fitness = importance[0][i] * importance[1][j] * importance[2][k];
                fitness_heap.push(pair);
            }
        }
    }

    Utils::sleep(5000);
    cout << "TEST CHECKS" << endl;

    unsigned int count = 0;
    while (!(sink->empty() && sink->finished())) {
        if (sink->empty()) {
            Utils::sleep();
            continue;
        }
        EXPECT_FALSE((query_answer = dynamic_cast<QueryAnswer*>(sink->pop())) == NULL);
        pair = fitness_heap.top();
        cout << count << " CHECK: " << query_answer->importance << " " << pair.importance << " ("
             << pair.fitness << ")" << endl;
        EXPECT_TRUE(double_equals(query_answer->importance, pair.importance));
        fitness_heap.pop();
        count++;
    }

    Utils::sleep(5000);
    cout << "TEAR DOWN" << endl;
}

TEST(AndOperator, empty_source) {
    auto source1 = make_shared<TestSource>();
    auto source2 = make_shared<TestSource>();
    auto and_operator = make_shared<And<2>>(array<shared_ptr<QueryElement>, 2>({source1, source2}));
    TestSink sink(and_operator);
    Utils::sleep(1000);

    EXPECT_TRUE(sink.empty());
    EXPECT_FALSE(sink.finished());

    source2->add("h2_0", 0.3, {"v1_1"}, {"2"});
    source2->add("h2_1", 0.2, {"v2_1"}, {"1"});
    EXPECT_TRUE(sink.empty());
    EXPECT_FALSE(sink.finished());
    source1->query_answers_finished();
    source2->query_answers_finished();
    Utils::sleep(SLEEP_DURATION);
    EXPECT_TRUE(sink.empty());
    EXPECT_TRUE(sink.finished());
}

TEST(AndOperator, not_operator_1) {
    array<shared_ptr<TestSource>, 3> source;
    source[0] = make_shared<TestSource>();
    source[1] = make_shared<TestSource>();
    source[2] = make_shared<TestSource>();
    vector<shared_ptr<QueryElement>> dummy;
    auto and_operator = make_shared<And<3>>(array<shared_ptr<QueryElement>, 3>({source[0], source[1], source[2]}), dummy, true);
    TestSink sink(and_operator);
    QueryAnswer* query_answer;

    source[0]->add("S0_1", 0.9, {"v1"}, {"1"});
    source[0]->add("S0_2", 0.8, {"v1"}, {"2"});
    source[0]->add("S0_3", 0.7, {"v1"}, {"3"});
    source[1]->add("S1_1", 0.3, {"v2"}, {"1"});
    source[1]->add("S1_2", 0.2, {"v2"}, {"2"});
    source[1]->add("S1_3", 0.1, {"v2"}, {"3"});

    source[2]->add("S2_1", 1.0, {"v1"}, {"2"});
    unsigned int expected_count = 6;

    Utils::sleep(SLEEP_DURATION);
    source[0]->query_answers_finished();
    source[1]->query_answers_finished();
    source[2]->query_answers_finished();

    unsigned int count = 0;
    while (count < expected_count) {
        if (sink.empty()) {
            Utils::sleep();
            continue;
        }
        if (sink.finished() && sink.empty()) {
            break;
        }
        EXPECT_FALSE((query_answer = dynamic_cast<QueryAnswer*>(sink.pop())) == NULL);
        LOG_INFO("query_answer: " + query_answer->to_string());
        EXPECT_EQ(query_answer->get_handles_vector().size(), 2);
        for (string handle : query_answer->get_handles_vector()) {
            EXPECT_NE(handle, string("S0_2"));
        }
        count++;
    }
    EXPECT_EQ(count, expected_count);
    Utils::sleep(SLEEP_DURATION);
    EXPECT_TRUE(sink.empty());
    EXPECT_TRUE(sink.finished());
}

TEST(AndOperator, not_operator_2) {
    array<shared_ptr<TestSource>, 3> source;
    source[0] = make_shared<TestSource>();
    source[1] = make_shared<TestSource>();
    source[2] = make_shared<TestSource>();
    vector<shared_ptr<QueryElement>> dummy;
    auto and_operator = make_shared<And<3>>(array<shared_ptr<QueryElement>, 3>({source[0], source[1], source[2]}), dummy, true);
    TestSink sink(and_operator);
    QueryAnswer* query_answer;

    source[0]->add("S0_1", 0.9, {"v1"}, {"1"});
    source[0]->add("S0_2", 0.8, {"v1"}, {"2"});
    source[0]->add("S0_3", 0.7, {"v1"}, {"3"});
    source[1]->add("S1_1", 0.3, {"v2"}, {"1"});
    source[1]->add("S1_2", 0.2, {"v2"}, {"2"});
    source[1]->add("S1_3", 0.1, {"v2"}, {"3"});

    source[2]->add("S2_1", 1.0, {"v1", "v2"}, {"2", "3"});
    unsigned int expected_count = 8;

    Utils::sleep(SLEEP_DURATION);
    source[0]->query_answers_finished();
    source[1]->query_answers_finished();
    source[2]->query_answers_finished();

    unsigned int count = 0;
    while (count < expected_count) {
        if (sink.empty()) {
            Utils::sleep();
            continue;
        }
        if (sink.finished() && sink.empty()) {
            break;
        }
        EXPECT_FALSE((query_answer = dynamic_cast<QueryAnswer*>(sink.pop())) == NULL);
        LOG_INFO("query_answer: " + query_answer->to_string());
        EXPECT_EQ(query_answer->get_handles_vector().size(), 2);
        auto handles = query_answer->get_handles_vector();
        EXPECT_FALSE((handles[0] == "S0_2") && (handles[1] == "S1_3"));
        count++;
    }
    EXPECT_EQ(count, expected_count);
    Utils::sleep(SLEEP_DURATION);
    EXPECT_TRUE(sink.empty());
    EXPECT_TRUE(sink.finished());
}

TEST(AndOperator, not_operator_3) {
    array<shared_ptr<TestSource>, 3> source;
    source[0] = make_shared<TestSource>();
    source[1] = make_shared<TestSource>();
    source[2] = make_shared<TestSource>();
    vector<shared_ptr<QueryElement>> dummy;
    auto and_operator = make_shared<And<3>>(array<shared_ptr<QueryElement>, 3>({source[0], source[1], source[2]}), dummy, true);
    TestSink sink(and_operator);
    QueryAnswer* query_answer;

    source[0]->add("S0_1", 0.9, {"v1"}, {"1"});
    source[0]->add("S0_2", 0.8, {"v1"}, {"2"});
    source[0]->add("S0_3", 0.7, {"v1"}, {"3"});
    source[1]->add("S1_1", 0.3, {"v2"}, {"1"});
    source[1]->add("S1_2", 0.2, {"v2"}, {"2"});
    source[1]->add("S1_3", 0.1, {"v2"}, {"3"});

    source[2]->add("S2_1", 1.0, {"v3"}, {"0"});
    unsigned int expected_count = 0;

    Utils::sleep(SLEEP_DURATION);
    source[0]->query_answers_finished();
    source[1]->query_answers_finished();
    source[2]->query_answers_finished();

    while (!(sink.empty() && sink.finished())) {
        if (sink.empty()) {
            Utils::sleep();
            continue;
        }
        FAIL();
    }
}

TEST(AndOperator, not_operator_4) {
    array<shared_ptr<TestSource>, 3> source;
    source[0] = make_shared<TestSource>();
    source[1] = make_shared<TestSource>();
    source[2] = make_shared<TestSource>();
    vector<shared_ptr<QueryElement>> dummy;
    auto and_operator = make_shared<And<3>>(array<shared_ptr<QueryElement>, 3>({source[0], source[1], source[2]}), dummy, true);
    TestSink sink(and_operator);
    QueryAnswer* query_answer;

    source[0]->add("S0_1", 0.9, {"v1"}, {"1"});
    source[0]->add("S0_2", 0.8, {"v1"}, {"2"});
    source[0]->add("S0_3", 0.7, {"v1"}, {"3"});
    source[1]->add("S1_1", 0.3, {"v2"}, {"1"});
    source[1]->add("S1_2", 0.2, {"v2"}, {"2"});
    source[1]->add("S1_3", 0.1, {"v2"}, {"3"});

    source[2]->add("S2_1", 1.0, {"v1"}, {"2"});
    source[2]->add("S2_2", 1.0, {"v2"}, {"3"});
    unsigned int expected_count = 4;

    Utils::sleep(SLEEP_DURATION);
    source[0]->query_answers_finished();
    source[1]->query_answers_finished();
    source[2]->query_answers_finished();

    unsigned int count = 0;
    while (count < expected_count) {
        if (sink.empty()) {
            Utils::sleep();
            continue;
        }
        if (sink.finished() && sink.empty()) {
            break;
        }
        EXPECT_FALSE((query_answer = dynamic_cast<QueryAnswer*>(sink.pop())) == NULL);
        LOG_INFO("query_answer: " + query_answer->to_string());
        EXPECT_EQ(query_answer->get_handles_vector().size(), 2);
        auto handles = query_answer->get_handles_vector();
        EXPECT_FALSE((handles[0] == "S0_2") || (handles[1] == "S1_3"));
        count++;
    }
    EXPECT_EQ(count, expected_count);
    Utils::sleep(SLEEP_DURATION);
    EXPECT_TRUE(sink.empty());
    EXPECT_TRUE(sink.finished());
}

TEST(AndOperator, not_operator_5) {
    array<shared_ptr<TestSource>, 3> source;
    source[0] = make_shared<TestSource>();
    source[1] = make_shared<TestSource>();
    source[2] = make_shared<TestSource>();
    vector<shared_ptr<QueryElement>> dummy;
    auto and_operator = make_shared<And<3>>(array<shared_ptr<QueryElement>, 3>({source[0], source[1], source[2]}), dummy, true);
    TestSink sink(and_operator);
    QueryAnswer* query_answer;

    source[0]->add("S0_1", 0.9, {"v1"}, {"1"});
    source[0]->add("S0_2", 0.8, {"v1"}, {"2"});
    source[0]->add("S0_3", 0.7, {"v1"}, {"3"});
    source[1]->add("S1_1", 0.3, {"v2"}, {"1"});
    source[1]->add("S1_2", 0.2, {"v2"}, {"2"});
    source[1]->add("S1_3", 0.1, {"v2"}, {"3"});

    unsigned int expected_count = 9;

    Utils::sleep(SLEEP_DURATION);
    source[0]->query_answers_finished();
    source[1]->query_answers_finished();
    source[2]->query_answers_finished();

    unsigned int count = 0;
    while (count < expected_count) {
        if (sink.empty()) {
            Utils::sleep();
            continue;
        }
        if (sink.finished() && sink.empty()) {
            break;
        }
        EXPECT_FALSE((query_answer = dynamic_cast<QueryAnswer*>(sink.pop())) == NULL);
        LOG_INFO("query_answer: " + query_answer->to_string());
        EXPECT_EQ(query_answer->get_handles_vector().size(), 2);
        count++;
    }
    EXPECT_EQ(count, expected_count);
    Utils::sleep(SLEEP_DURATION);
    EXPECT_TRUE(sink.empty());
    EXPECT_TRUE(sink.finished());
}
