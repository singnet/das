#include <cstdlib>
#include <cstring>

#include "AtomDBSingleton.h"
#include "Chain.h"
#include "Hasher.h"
#include "InMemoryDB.h"
#include "Logger.h"
#include "QueryAnswer.h"
#include "Sink.h"
#include "Source.h"
#include "gtest/gtest.h"
#include "test_utils.h"

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

// Just to help in debugging
// clang-format off
#define RUN_allow_concatenation         ((bool) true)
#define RUN_allow_concatenation_reverse ((bool) true)
#define RUN_back_after_dead_end         ((bool) true)
#define RUN_basics                      ((bool) true)
#define RUN_complete_only               ((bool) true)
// clang-format on

static string EVALUATION_HANDLE = Hasher::node_handle(NODE_TYPE, EVALUATION);

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
        atoms::Link* link;
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
                link = new atoms::Link(
                    LINK_TYPE, {EVALUATION_HANDLE, node1->handle(), node2->handle()}, true);
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

    void TearDown() override {}
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
            string link_handle =
                Hasher::link_handle(LINK_TYPE, {EVALUATION_HANDLE, node1_handle, node2_handle});
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
    Link link(LINK_TYPE, {EVALUATION_HANDLE, node1.handle(), node2.handle()});
    ALL_LINKS.insert(link.handle());
    return link.handle();
}

static string link(unsigned int node1, unsigned int node2) {
    return link(node_name(node1), node_name(node2));
}

class TestSource : public Source {
   public:
    TestSource() { this->id = "TestSource_" + Utils::random_string(30); }

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
    TestSink(shared_ptr<QueryElement> precedent)
        : Sink(precedent, "TestSink(" + precedent->id + ", " + Utils::random_string(30) + ")") {}
    ~TestSink() {}
    bool empty() { return this->input_buffer->is_query_answers_empty(); }
    bool finished() { return this->input_buffer->is_query_answers_finished(); }
    QueryAnswer* pop() { return this->input_buffer->pop_query_answer(); }
};

static bool check_answer(QueryAnswer* query_answer) {
    string origin = get_node_string(query_answer->get(Chain::ORIGIN_VARIABLE_NAME));
    string destiny = get_node_string(query_answer->get(Chain::DESTINY_VARIABLE_NAME));
    EXPECT_TRUE((origin == "S") || (origin == "T"));
    if (origin == "S") {
        EXPECT_TRUE(get_link_string(query_answer->get_handles_vector().front(), 1) == "S");
    } else {
        EXPECT_TRUE(get_link_string(query_answer->get_handles_vector().back(), 2) == "T");
    }
    bool first = true;
    string cursor;
    for (string handle : query_answer->get_handles_vector()) {
        EXPECT_TRUE(ALL_LINKS.find(handle) != ALL_LINKS.end());
        if (first) {
            first = false;
            cursor = get_link_string(handle, 1);
        }
        EXPECT_EQ(get_link_string(handle, 1), cursor);
        cursor = get_link_string(handle, 2);
    }
    return (((origin == "S") && (destiny == "T")) || ((origin == "T") && (destiny == "S")));
}

static string answer_path_to_string(QueryAnswer* query_answer) {
    bool first = true;
    string answer = "";
    string cursor;
    for (string handle : query_answer->get_handles_vector()) {
        if (first) {
            first = false;
            answer = cursor = get_link_string(handle, 1);
        }
        if (get_link_string(handle, 1) != cursor) {
            return "<Invalid path " + query_answer->to_string() + " " + answer + " + " +
                   get_link_string(handle) + ">";
        }
        answer += " -> ";
        cursor = get_link_string(handle, 2);
        answer += cursor;
    }
    return answer;
}

TEST(ChainOperatorTest, allow_concatenation) {
    if (!RUN_allow_concatenation) return;

    shared_ptr<Link> ab_link(new Link(LINK_TYPE, {" ", "a", "b"}));
    shared_ptr<Link> ba_link(new Link(LINK_TYPE, {" ", "b", "a"}));
    shared_ptr<Link> bc_link(new Link(LINK_TYPE, {" ", "b", "c"}));
    shared_ptr<Link> ca_link(new Link(LINK_TYPE, {" ", "c", "a"}));
    shared_ptr<Link> cd_link(new Link(LINK_TYPE, {" ", "c", "d"}));
    shared_ptr<Link> da_link(new Link(LINK_TYPE, {" ", "d", "a"}));
    shared_ptr<Link> db_link(new Link(LINK_TYPE, {" ", "d", "b"}));
    shared_ptr<Link> dc_link(new Link(LINK_TYPE, {" ", "d", "c"}));
    shared_ptr<Link> dd_link(new Link(LINK_TYPE, {" ", "d", "d"}));
    shared_ptr<Link> xa_link(new Link(LINK_TYPE, {" ", "x", "a"}));
    shared_ptr<Link> xb_link(new Link(LINK_TYPE, {" ", "x", "b"}));
    shared_ptr<Link> xc_link(new Link(LINK_TYPE, {" ", "x", "c"}));
    shared_ptr<Link> xe_link(new Link(LINK_TYPE, {" ", "x", "e"}));
    shared_ptr<Link> dx_link(new Link(LINK_TYPE, {" ", "d", "x"}));

    Chain::Path base(true);
    Chain::Path new_path(true);
    Chain::Path ab(ab_link, new QueryAnswer(0), true);
    Chain::Path ba(ba_link, new QueryAnswer(0), true);
    Chain::Path bc(bc_link, new QueryAnswer(0), true);
    Chain::Path ca(ca_link, new QueryAnswer(0), true);
    Chain::Path cd(cd_link, new QueryAnswer(0), true);
    Chain::Path da(da_link, new QueryAnswer(0), true);
    Chain::Path db(db_link, new QueryAnswer(0), true);
    Chain::Path dc(dc_link, new QueryAnswer(0), true);
    EXPECT_THROW(Chain::Path dd(dd_link, new QueryAnswer(0), true), runtime_error);
    Chain::Path xa(xa_link, new QueryAnswer(0), true);
    Chain::Path xb(xb_link, new QueryAnswer(0), true);
    Chain::Path xc(xc_link, new QueryAnswer(0), true);
    Chain::Path xe(xe_link, new QueryAnswer(0), true);
    Chain::Path dx(dx_link, new QueryAnswer(0), true);

    base.clear();
    EXPECT_TRUE(base.allow_concatenation(ab));
    EXPECT_TRUE(base.allow_concatenation(ba));
    base.concatenate(ab);
    EXPECT_FALSE(base.allow_concatenation(ab));
    EXPECT_FALSE(base.allow_concatenation(ba));

    base.clear();
    EXPECT_TRUE(base.allow_concatenation(ab));
    EXPECT_TRUE(base.allow_concatenation(bc));
    EXPECT_TRUE(base.allow_concatenation(ca));
    base.concatenate(ab);
    EXPECT_FALSE(base.allow_concatenation(ab));
    EXPECT_TRUE(base.allow_concatenation(bc));
    EXPECT_FALSE(base.allow_concatenation(ca));
    base.concatenate(bc);
    EXPECT_FALSE(base.allow_concatenation(ab));
    EXPECT_FALSE(base.allow_concatenation(bc));
    EXPECT_FALSE(base.allow_concatenation(ca));

    base.clear();
    base.concatenate(ab);
    base.concatenate(bc);
    base.concatenate(cd);
    EXPECT_FALSE(base.allow_concatenation(da));
    EXPECT_FALSE(base.allow_concatenation(db));
    EXPECT_FALSE(base.allow_concatenation(dc));

    base.clear();
    base.concatenate(ab);
    base.concatenate(bc);
    base.concatenate(cd);
    for (auto hop : {xa, xb, xc}) {
        new_path.clear();
        new_path.concatenate(dx);
        new_path.concatenate(hop);
        EXPECT_FALSE(base.allow_concatenation(new_path));
    }
    new_path.clear();
    new_path.concatenate(dx);
    new_path.concatenate(xe);
    EXPECT_TRUE(base.allow_concatenation(new_path));

    EXPECT_TRUE(base.contains("a"));
    EXPECT_TRUE(base.contains("b"));
    EXPECT_TRUE(base.contains("c"));
    EXPECT_TRUE(base.contains("d"));
    EXPECT_FALSE(base.contains("x"));
    EXPECT_FALSE(base.contains("e"));
    base.concatenate(new_path);
    EXPECT_TRUE(base.contains("a"));
    EXPECT_TRUE(base.contains("b"));
    EXPECT_TRUE(base.contains("c"));
    EXPECT_TRUE(base.contains("d"));
    EXPECT_TRUE(base.contains("x"));
    EXPECT_TRUE(base.contains("e"));
}

TEST(ChainOperatorTest, allow_concatenation_reverse) {
    if (!RUN_allow_concatenation_reverse) return;

    return;
    shared_ptr<Link> ab_link(new Link(LINK_TYPE, {" ", "a", "b"}));
    shared_ptr<Link> bc_link(new Link(LINK_TYPE, {" ", "b", "c"}));
    shared_ptr<Link> cd_link(new Link(LINK_TYPE, {" ", "c", "d"}));
    shared_ptr<Link> dd_link(new Link(LINK_TYPE, {" ", "d", "d"}));
    shared_ptr<Link> dx_link(new Link(LINK_TYPE, {" ", "d", "x"}));
    shared_ptr<Link> cx_link(new Link(LINK_TYPE, {" ", "c", "x"}));
    shared_ptr<Link> bx_link(new Link(LINK_TYPE, {" ", "b", "x"}));
    shared_ptr<Link> ex_link(new Link(LINK_TYPE, {" ", "e", "x"}));
    shared_ptr<Link> xa_link(new Link(LINK_TYPE, {" ", "x", "a"}));

    Chain::Path base(false);
    Chain::Path new_path(false);
    Chain::Path ba(ab_link, new QueryAnswer(0), false);
    Chain::Path cb(bc_link, new QueryAnswer(0), false);
    Chain::Path dc(cd_link, new QueryAnswer(0), false);
    EXPECT_THROW(Chain::Path dd(dd_link, new QueryAnswer(0), false), runtime_error);
    Chain::Path xd(dx_link, new QueryAnswer(0), false);
    Chain::Path xc(cx_link, new QueryAnswer(0), false);
    Chain::Path xb(bx_link, new QueryAnswer(0), false);
    Chain::Path xe(ex_link, new QueryAnswer(0), false);
    Chain::Path ax(xa_link, new QueryAnswer(0), false);

    base.clear();
    base.concatenate(dc);
    base.concatenate(cb);
    base.concatenate(ba);
    for (auto hop : {xd, xc, xb}) {
        new_path.clear();
        new_path.concatenate(ax);
        new_path.concatenate(hop);
        EXPECT_FALSE(base.allow_concatenation(new_path));
    }
    new_path.clear();
    new_path.concatenate(ax);
    new_path.concatenate(xe);
    EXPECT_TRUE(base.allow_concatenation(new_path));

    EXPECT_TRUE(base.contains("a"));
    EXPECT_TRUE(base.contains("b"));
    EXPECT_TRUE(base.contains("c"));
    EXPECT_TRUE(base.contains("d"));
    EXPECT_FALSE(base.contains("x"));
    EXPECT_FALSE(base.contains("e"));
    base.concatenate(new_path);
    EXPECT_TRUE(base.contains("a"));
    EXPECT_TRUE(base.contains("b"));
    EXPECT_TRUE(base.contains("c"));
    EXPECT_TRUE(base.contains("d"));
    EXPECT_TRUE(base.contains("x"));
    EXPECT_TRUE(base.contains("e"));
}

TEST(ChainOperatorTest, back_after_dead_end) {
    if (!RUN_back_after_dead_end) return;
    auto source = make_shared<TestSource>();
    auto chain_operator = make_shared<Chain>(array<shared_ptr<QueryElement>, 1>({source}),
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
    //         +X 9 --+-- 10 -+- 13       |  |
    //         |      |       |           |  |
    //         |      |       +- 14 X 19 -+  |
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
    source->add(link(15, T),  0.5, {"v1"}, {"h1"}, false);
    source->add(link(17, 9),  0.5, {"v1"}, {"h1"}, false);
    source->add(link(18, 12), 0.5, {"v1"}, {"h1"}, false);
    source->add(link(19, T),  0.5, {"v1"}, {"h1"}, false);
    source->add(link(20, T),  0.5, {"v1"}, {"h1"}, false);
    Utils::sleep(3000); // TODO remove this
    // clang-format on
    QueryAnswer* answer;
    unsigned int complete_path = 0;
    while (complete_path < 5) {
        while ((answer = sink.pop()) != NULL) {
            LOG_INFO("[" + std::to_string(answer->importance) + "]: " + answer_path_to_string(answer));
            if (check_answer(answer)) {
                complete_path++;
            }
        }
        Utils::sleep(500);
    }
    EXPECT_FALSE(sink.finished());
    EXPECT_EQ(complete_path, 5);
    source->add(link(S, 9), 0.5, {"v1"}, {"h1"}, false);
    while (complete_path < 6) {
        while ((answer = sink.pop()) != NULL) {
            LOG_INFO("[" + std::to_string(answer->importance) + "]: " + answer_path_to_string(answer));
            if (check_answer(answer)) {
                complete_path++;
            }
        }
        Utils::sleep(500);
    }
    EXPECT_FALSE(sink.finished());
    EXPECT_EQ(complete_path, 6);
    source->add(link(14, 19), 0.5, {"v1"}, {"h1"}, false);
    source->query_answers_finished();
    while (!sink.empty() || !sink.finished()) {
        while ((answer = sink.pop()) != NULL) {
            LOG_INFO("[" + std::to_string(answer->importance) + "]: " + answer_path_to_string(answer));
            if (check_answer(answer)) {
                complete_path++;
            }
        }
        Utils::sleep(500);
    }
    EXPECT_EQ(complete_path, 7);
    EXPECT_TRUE(sink.empty());
    EXPECT_TRUE(sink.finished());
}

TEST(ChainOperatorTest, complete_only) {
    if (!RUN_complete_only) return;
    auto source = make_shared<TestSource>();
    auto chain_operator = make_shared<Chain>(array<shared_ptr<QueryElement>, 1>({source}),
                                             Hasher::node_handle(NODE_TYPE, "S"),
                                             Hasher::node_handle(NODE_TYPE, "T"),
                                             false);
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
    source->add(link(17, 9),  0.5, {"v1"}, {"h1"}, false);
    source->add(link(18, 12), 0.5, {"v1"}, {"h1"}, false);
    source->add(link(19, T),  0.5, {"v1"}, {"h1"}, false);
    source->add(link(20, T),  0.5, {"v1"}, {"h1"}, false);
    Utils::sleep(3000); // TODO remove this
    // clang-format on
    source->query_answers_finished();
    QueryAnswer* answer;
    unsigned int complete_path = 0;
    while (!sink.empty() || !sink.finished()) {
        while ((answer = sink.pop()) != NULL) {
            LOG_INFO("[" + std::to_string(answer->importance) + "]: " + answer_path_to_string(answer));
            EXPECT_TRUE(check_answer(answer));
            complete_path++;
        }
        Utils::sleep(500);
    }
    EXPECT_EQ(complete_path, 7);
    EXPECT_TRUE(sink.empty());
    EXPECT_TRUE(sink.finished());
}

TEST(ChainOperatorTest, basics) {
    if (!RUN_basics) return;
    auto source = make_shared<TestSource>();
    auto chain_operator = make_shared<Chain>(array<shared_ptr<QueryElement>, 1>({source}),
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
    source->add(link(17, 9),  0.5, {"v1"}, {"h1"}, false);
    source->add(link(18, 12), 0.5, {"v1"}, {"h1"}, false);
    source->add(link(19, T),  0.5, {"v1"}, {"h1"}, false);
    source->add(link(20, T),  0.5, {"v1"}, {"h1"}, false);
    Utils::sleep(3000); // TODO remove this
    // clang-format on
    source->query_answers_finished();
    QueryAnswer* answer;
    unsigned int complete_path = 0;
    while (!sink.empty() || !sink.finished()) {
        while ((answer = sink.pop()) != NULL) {
            LOG_INFO("[" + std::to_string(answer->importance) + "]: " + answer_path_to_string(answer));
            if (check_answer(answer)) {
                complete_path++;
            }
        }
        Utils::sleep(500);
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
