#include <cstdlib>
#include <cstring>

#include "Chain.h"
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

    ~TestSource() {}

    void add(const string& handle,
             double importance,
             const array<const char*, 1>& labels,
             const array<const char*, 1>& values,
             bool sleep_flag = true) {
        QueryAnswer* query_answer = new QueryAnswer(handle, importance);
        /*
        for (unsigned int i = 0; i < labels.size(); i++) {
            query_answer->assignment.assign(labels[i], values[i]);
        }
        */
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
    EXPECT_EQ(query_answer->handles.size(), 2);
    set<string> set_handles;
    for (unsigned int i = 0; i < handles.size(); i++) {
        set_handles.insert(handles[i]);
    }
    EXPECT_TRUE(set<string>(query_answer->handles.begin(), query_answer->handles.end()) == set_handles);
}

static char LINK_HANDLE_DELIMITER  = '-';

static string link(const string& node1, const string& node2) {
    return node1 + std::to_string(LINK_HANDLE_DELIMITER) + node2;
}

static string link(unsigned int node1, unsigned int node2) {
    return link(std::to_string(node1), std::to_string(node2));
}

static vector<string> targets(const string& link_handle) {
    return Utils::split(link_handle, LINK_HANDLE_DELIMITER);
}

TEST(ChainOperator, basics) {
    auto source = make_shared<TestSource>(10);
    auto chain_operator = make_shared<Chain>(array<shared_ptr<QueryElement>, 1>({source}));
    TestSink sink(chain_operator);

    EXPECT_TRUE(sink.empty());
    EXPECT_FALSE(sink.finished());

    vector<string> node;
    unsigned int S = 0;
    unsigned int node_count = 20;
    unsigned int T = node_count + 1;
    node.push_back("S");
    for (unsigned int cursor = S + 1; cursor <= node_count; cursor++) {
        node.push_back(std::to_string(cursor));
    }
    node.push_back("T");
 
    // clang-format off
    //
    //         +----- 1 -- 2 -- 3 --------+
    //         |      |    |              |
    //         +- 4 --+    |              |
    //         |           |              |
    //         +-----------+              |
    //         |                          |
    //         +- 5 --+-- 7               |
    //         |      |                   |
    //         +      +-- 6 -- 8          |
    //         |                          |
    //  S -----+--------------------------+--+---- T
    //         |                          |  |
    //         |                          |  |
    //         +- 9 --+-- 10 -+- 13       |  |
    //         |      |       |           |  |
    //         |      |       +- 14 - 19 -+  |
    //         |      |                      |
    //         |      +-- 11 -+- 15 ---------+
    //         |      |       |              |
    //         |      |       +- 16          |
    //         |      |                      |
    //         |      +-- 12 -+- 17 -> 9     |
    //         |              |              |
    //         |              +- 18 -> 12    |
    //         |                             |
    //         +------------------------ 20 -+

    source->add(link(S, 1),   0.5, {}, {}, false);
    source->add(link(S, 2),   0.5, {}, {}, false);
    source->add(link(S, T),   0.5, {}, {}, false);
    source->add(link(S, 4),   0.5, {}, {}, false);
    source->add(link(S, 5),   0.5, {}, {}, false);
    source->add(link(S, 9),   0.5, {}, {}, false);
    source->add(link(S, 20),  0.5, {}, {}, false);
    source->add(link(1, 2),   0.5, {}, {}, false);
    source->add(link(2, 3),   0.5, {}, {}, false);
    source->add(link(3, T),   0.5, {}, {}, false);
    source->add(link(4, 1),   0.5, {}, {}, false);
    source->add(link(5, 7),   0.5, {}, {}, false);
    source->add(link(5, 6),   0.5, {}, {}, false);
    source->add(link(6, 8),   0.5, {}, {}, false);
    source->add(link(9, 10),  0.5, {}, {}, false);
    source->add(link(9, 11),  0.5, {}, {}, false);
    source->add(link(9, 12),  0.5, {}, {}, false);
    source->add(link(10, 13), 0.5, {}, {}, false);
    source->add(link(10, 14), 0.5, {}, {}, false);
    source->add(link(11, 15), 0.5, {}, {}, false);
    source->add(link(11, 16), 0.5, {}, {}, false);
    source->add(link(12, 17), 0.5, {}, {}, false);
    source->add(link(12, 18), 0.5, {}, {}, false);
    source->add(link(14, 19), 0.5, {}, {}, false);
    source->add(link(15, T),  0.5, {}, {}, false);
    source->add(link(17, 9),  0.5, {}, {}, false);
    source->add(link(18, 12), 0.5, {}, {}, false);
    source->add(link(19, T),  0.5, {}, {}, false);
    source->add(link(20, T),  0.5, {}, {}, false);
    // clang-format on
}
