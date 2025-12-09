#include "Processor.h"

#include <gtest/gtest.h>

using namespace std;
using namespace processor;

TEST(ProcessorTest, basics) {
    Processor p1("blah");
    EXPECT_THROW(Processor p2(""), runtime_error);
    EXPECT_EQ(p1.to_string(), "blah");

    EXPECT_FALSE(p1.is_setup());
    EXPECT_FALSE(p1.is_running());
    EXPECT_FALSE(p1.is_finished());

    EXPECT_THROW(p1.start(), runtime_error);
    EXPECT_THROW(p1.stop(), runtime_error);
    EXPECT_FALSE(p1.is_setup());
    EXPECT_FALSE(p1.is_running());
    EXPECT_FALSE(p1.is_finished());
    p1.setup();
    EXPECT_TRUE(p1.is_setup());
    EXPECT_FALSE(p1.is_running());
    EXPECT_FALSE(p1.is_finished());

    EXPECT_THROW(p1.setup(), runtime_error);
    EXPECT_THROW(p1.stop(), runtime_error);
    EXPECT_TRUE(p1.is_setup());
    EXPECT_FALSE(p1.is_running());
    EXPECT_FALSE(p1.is_finished());
    p1.start();
    EXPECT_TRUE(p1.is_setup());
    EXPECT_TRUE(p1.is_running());
    EXPECT_FALSE(p1.is_finished());

    EXPECT_THROW(p1.setup(), runtime_error);
    EXPECT_THROW(p1.start(), runtime_error);
    EXPECT_TRUE(p1.is_setup());
    EXPECT_TRUE(p1.is_running());
    EXPECT_FALSE(p1.is_finished());
    p1.stop();
    EXPECT_TRUE(p1.is_setup());
    EXPECT_FALSE(p1.is_running());
    EXPECT_TRUE(p1.is_finished());

    EXPECT_THROW(p1.setup(), runtime_error);
    EXPECT_THROW(p1.start(), runtime_error);
    EXPECT_THROW(p1.stop(), runtime_error);
    EXPECT_TRUE(p1.is_setup());
    EXPECT_FALSE(p1.is_running());
    EXPECT_TRUE(p1.is_finished());
}

TEST(ProcessorTest, subprocessors) {
    shared_ptr<Processor> a, b, c, d, e, f, g, h;
    vector<shared_ptr<Processor>> nodes;
    shared_ptr<Processor> p;
    for (unsigned int i = 0; i < 8; i++) {
        a = make_shared<Processor>("a");
        b = make_shared<Processor>("b");
        c = make_shared<Processor>("c");
        d = make_shared<Processor>("d");
        e = make_shared<Processor>("e");
        f = make_shared<Processor>("f");
        g = make_shared<Processor>("g");
        h = make_shared<Processor>("h");
        nodes = {a, b, c, d, e, f, g, h};
        Processor::bind_subprocessor(a, c);  //    a       //
        Processor::bind_subprocessor(a, d);  //   / \      //
        Processor::bind_subprocessor(d, e);  //  c   d     //
        Processor::bind_subprocessor(d, f);  //    / | \   //
        Processor::bind_subprocessor(d, g);  //   e  f  g  //
        Processor::bind_subprocessor(e, b);  //   |  |     //
        Processor::bind_subprocessor(f, h);  //   b  h     //

        p = nodes[i % nodes.size()];
        for (auto cursor : nodes) {
            EXPECT_FALSE(cursor->is_setup());
            EXPECT_FALSE(cursor->is_running());
            EXPECT_FALSE(cursor->is_finished());
        }
        p->setup();
        for (auto cursor : nodes) {
            EXPECT_TRUE(cursor->is_setup());
            EXPECT_FALSE(cursor->is_running());
            EXPECT_FALSE(cursor->is_finished());
        }
        p->start();
        for (auto cursor : nodes) {
            EXPECT_TRUE(cursor->is_setup());
            EXPECT_TRUE(cursor->is_running());
            EXPECT_FALSE(cursor->is_finished());
        }
        p->stop();
        for (auto cursor : nodes) {
            EXPECT_TRUE(cursor->is_setup());
            EXPECT_FALSE(cursor->is_running());
            EXPECT_TRUE(cursor->is_finished());
        }
    }
}
