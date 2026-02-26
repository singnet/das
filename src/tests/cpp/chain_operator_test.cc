#include <cstdlib>
#include <cstring>

#include "Chain.h"
#include "AtomDBSingleton.h"
#include "InMemoryDB.h"
#include "QueryAnswer.h"
#include "Sink.h"
#include "Hasher.h"
#include "Source.h"
#include "gtest/gtest.h"
#include "test_utils.h"

#include "Logger.h"

using namespace commons;
using namespace query_engine;
using namespace query_element;
using namespace atomdb;
using namespace commons;

#define SLEEP_DURATION ((unsigned int) 1000)
#define NODE_TYPE "Node"
#define LINK_TYPE "Link"
#define EVALUATION "EVALUATION"
#define UNKNOWN_NODE "UNKNOWN_NODE";
#define UNKNOWN_LINK "UNKNOWN_LINK";
#define NODE_COUNT ((unsigned int) 20)

static string evaluation_handle = Hasher::node_handle(NODE_TYPE, EVALUATION);

static set<string> ALL_LINKS;

static string node_name(unsigned int n) {
    if (n == 0) {
        return "S";
    } else if (n == (NODE_COUNT + 1)) {
        return "T";
    } else {
        return std::to_string(n);
    }
}

class ChainOperatorTestEnvironment : public ::testing::Environment {
   public:

    void load_data() {
        auto db = AtomDBSingleton::get_instance();
        atoms::Node *node1, *node2;
        atoms::Link *link;
        node1 = new atoms::Node(NODE_TYPE, EVALUATION);
        LOG_DEBUG("Add node: " + node1->handle() + " " + node1->to_string());
        db->add_node(node1, false);
        for (unsigned int i = 0; i <= (NODE_COUNT + 1); i++) {
            node1 = new atoms::Node(NODE_TYPE, node_name(i));
            db->add_node(node1, false);
            LOG_DEBUG("Add node: " + node1->handle() + " " + node1->to_string());
            for (unsigned int j = 0; j <= (NODE_COUNT + 1); j++) {
                node2 = new atoms::Node(NODE_TYPE, node_name(j));
                LOG_DEBUG("Add node: " + node2->handle() + " " + node2->to_string());
                db->add_node(node2, false);
                link = new atoms::Link(LINK_TYPE, {evaluation_handle, node1->handle(), node2->handle()}, true);
                LOG_DEBUG("Add link: " + link->handle() + " " + link->to_string());
                db->add_link(link, false);
            }
        }
    }

    void SetUp() override {
        auto atomdb = new InMemoryDB("chain_operator_test_");
        AtomDBSingleton::provide(shared_ptr<AtomDB>(atomdb));
        this->load_data();
    }

    void TearDown() override {
    }
};

class ChainOperatorTest : public ::testing::Test {
   protected:
    void SetUp() override {
        auto atomdb = AtomDBSingleton::get_instance();
        db = dynamic_pointer_cast<InMemoryDB>(atomdb);
        ASSERT_NE(db, nullptr) << "Failed to cast AtomDB to InMemoryDB";
    }

    void TearDown() override {}
    shared_ptr<InMemoryDB> db;
};

static string get_node_string(string handle) {
    for (unsigned int i = 0; i <= (NODE_COUNT + 1); i++) {
        string node_handle = Hasher::node_handle(NODE_TYPE, node_name(i));
        if (node_handle == handle) {
            return node_name(i);
        }
    }
    return UNKNOWN_NODE;
}

static string get_link_string(string handle, unsigned int select = 0) {
    for (unsigned int i = 0; i <= (NODE_COUNT + 1); i++) {
        string node1_handle = Hasher::node_handle(NODE_TYPE, node_name(i));
        for (unsigned int j = 0; j <= (NODE_COUNT + 1); j++) {
            string node2_handle = Hasher::node_handle(NODE_TYPE, node_name(j));
            string link_handle = Hasher::link_handle(LINK_TYPE, {evaluation_handle, node1_handle, node2_handle});
            if (link_handle == handle) {
                if (select == 0) {
                    return get_node_string(node1_handle) + " -> " + get_node_string(node2_handle);
                } else if (select == 1) {
                    return get_node_string(node1_handle);
                } else {
                    return get_node_string(node2_handle);
                }
            }
        }
    }
    return UNKNOWN_LINK;
}

static string link(const string& node1_name, const string& node2_name) {
    Node node1(NODE_TYPE, node1_name);
    Node node2(NODE_TYPE, node2_name);
    Link link(LINK_TYPE, {evaluation_handle, node1.handle(), node2.handle()});
    ALL_LINKS.insert(link.handle());
    return link.handle();
}

static string link(unsigned int node1, unsigned int node2) {
    return link(node_name(node1), node_name(node2));
}

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
        for (unsigned int i = 0; i < labels.size(); i++) {
            query_answer->assignment.assign(labels[i], values[i]);
        }
        LOG_INFO("Feeding answer in the source element: " + get_link_string(handle));
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

static void check_query_answer(string tag,
                        QueryAnswer* query_answer,
                        double importance,
                        unsigned int handles_size,
                        const array<const char*, 2>& handles) {
    /*
    cout << "check_query_answer(" + tag + ")" << endl;
    EXPECT_TRUE(double_equals(query_answer->importance, importance));
    EXPECT_EQ(query_answer->handles.size(), 2);
    set<string> set_handles;
    for (unsigned int i = 0; i < handles.size(); i++) {
        set_handles.insert(handles[i]);
    }
    EXPECT_TRUE(set<string>(query_answer->handles.begin(), query_answer->handles.end()) == set_handles);
    */
}

static bool check_answer(QueryAnswer *query_answer) {
    string origin = get_node_string(query_answer->get(Chain::ORIGIN_VARIABLE_NAME));
    string destiny = get_node_string(query_answer->get(Chain::DESTINY_VARIABLE_NAME));
    EXPECT_TRUE((origin == "S") || (origin == "T"));
    if (origin == "S") {
        EXPECT_TRUE(get_link_string(query_answer->handles.front(), 1) == "S");
    } else {
        EXPECT_TRUE(get_link_string(query_answer->handles.back(), 2) == "T");
    }
    bool first = true;
    string cursor;
    for (string handle : query_answer->handles) {
        EXPECT_TRUE(ALL_LINKS.find(handle) != ALL_LINKS.end());
        if (first) {
            first = false;
            cursor = get_link_string(handle, 1);
        }
        EXPECT_EQ(get_link_string(handle, 1), cursor);
        cursor = get_link_string(handle, 2);
    }
    return (((origin == "S") && (destiny == "T")) ||
            ((origin == "T") && (destiny == "S")));
}

static string answer_path_to_string(QueryAnswer *query_answer) {
    bool first = true;
    string answer = "";
    string cursor;
    for (string handle : query_answer->handles) {
        if (first) {
            first = false;
            answer = cursor = get_link_string(handle, 1);
        }
        if (get_link_string(handle, 1) != cursor) {
            Utils::error("Invalid path: " + query_answer->to_string() + " " + answer + " + " + get_link_string(handle));
        }
        answer += " -> ";
        cursor = get_link_string(handle, 2);
        answer += cursor;
    }
    return answer;
}

TEST(ChainOperatorTest, basics) {
    auto source = make_shared<TestSource>(10);
    auto chain_operator = make_shared<Chain>(
        array<shared_ptr<QueryElement>, 1>({source}), 
        Hasher::node_handle(NODE_TYPE, "S"), 
        Hasher::node_handle(NODE_TYPE, "T"));
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
    //  S -----+--------------------------+--+----- T
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
    //         |      +-- 12 -+- 17 -> (9)   |
    //         |              |              |
    //         |              +- 18 -> (12)  |
    //         |                             |
    //         +------------------------ 20 -+

    Utils::sleep(1000);
    source->add(link(S, 1),   0.5, {"v1"}, {"h1"}, false);
    source->add(link(S, 2),   0.5, {"v1"}, {"h1"}, false);
    source->add(link(S, T),   0.5, {"v1"}, {"h1"}, false);
    source->add(link(S, 4),   0.5, {"v1"}, {"h1"}, false);
    source->add(link(S, 5),   0.5, {"v1"}, {"h1"}, false);
    source->add(link(S, 9),   0.5, {"v1"}, {"h1"}, false);
    source->add(link(S, 20),  0.5, {"v1"}, {"h1"}, false);
    source->add(link(1, 2),   0.5, {"v1"}, {"h1"}, false);
    source->add(link(2, 3),   0.5, {"v1"}, {"h1"}, false);
    source->add(link(3, T),   0.5, {"v1"}, {"h1"}, false);
    source->add(link(4, 1),   0.5, {"v1"}, {"h1"}, false);
    source->add(link(5, 7),   0.5, {"v1"}, {"h1"}, false);
    source->add(link(5, 6),   0.5, {"v1"}, {"h1"}, false);
    source->add(link(6, 8),   0.5, {"v1"}, {"h1"}, false);
    source->add(link(9, 10),  0.5, {"v1"}, {"h1"}, false);
    source->add(link(9, 11),  0.5, {"v1"}, {"h1"}, false);
    source->add(link(9, 12),  0.5, {"v1"}, {"h1"}, false);
    source->add(link(10, 13), 0.5, {"v1"}, {"h1"}, false);
    source->add(link(10, 14), 0.5, {"v1"}, {"h1"}, false);
    source->add(link(11, 15), 0.5, {"v1"}, {"h1"}, false);
    source->add(link(11, 16), 0.5, {"v1"}, {"h1"}, false);
    source->add(link(12, 17), 0.5, {"v1"}, {"h1"}, false);
    source->add(link(12, 18), 0.5, {"v1"}, {"h1"}, false);
    source->add(link(14, 19), 0.5, {"v1"}, {"h1"}, false);
    source->add(link(15, T),  0.5, {"v1"}, {"h1"}, false);
    //source->add(link(17, 9),  0.5, {"v1"}, {"h1"}, false);
    //source->add(link(18, 12), 0.5, {"v1"}, {"h1"}, false);
    source->add(link(19, T),  0.5, {"v1"}, {"h1"}, false);
    source->add(link(20, T),  0.5, {"v1"}, {"h1"}, false);
    // clang-format on
    source->query_answers_finished();
    QueryAnswer *answer;
    unsigned int complete_path = 0;
    Utils::sleep(3000);
    while (! sink.empty() || ! sink.finished()) {
        while ((answer = sink.pop()) != NULL) {
            LOG_INFO("[" + std::to_string(answer->importance) + "]: " + answer_path_to_string(answer));
            if (check_answer(answer)) {
                complete_path++;
            }
        }
    }
    EXPECT_EQ(complete_path, 7);
    EXPECT_TRUE(sink.empty());
    EXPECT_TRUE(sink.finished());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new ChainOperatorTestEnvironment());
    return RUN_ALL_TESTS();
}
