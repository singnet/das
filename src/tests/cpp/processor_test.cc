#include "Processor.h"

#include <gtest/gtest.h>

#include "DedicatedThread.h"
#include "Logger.h"
#include "Utils.h"
#include "processor/ThreadPool.h"

using namespace std;
using namespace processor;
using namespace commons;

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

TEST(ProcessorTest, dedicated_thread) {
    class TestThreadMethod : public ThreadMethod {
       public:
        mutex api_mutex;
        bool cycle_flag;
        unsigned int count;
        TestThreadMethod() {
            this->count = 0;
            this->cycle_flag = true;
        }
        void set_cycle_flag(bool value) {
            lock_guard<mutex> semaphore(this->api_mutex);
            this->cycle_flag = value;
        }
        bool get_cycle_flag() {
            lock_guard<mutex> semaphore(this->api_mutex);
            return this->cycle_flag;
        }
        unsigned int get_count() {
            lock_guard<mutex> semaphore(this->api_mutex);
            return this->count;
        }
        bool thread_one_step() {
            if (get_cycle_flag()) {
                this->api_mutex.lock();
                this->count++;
                this->api_mutex.unlock();
                set_cycle_flag(false);
            }
            return false;
        }
    };

    TestThreadMethod thread_method;
    DedicatedThread dedicated_thread("blah", &thread_method);
    EXPECT_EQ(thread_method.get_count(), 0);
    dedicated_thread.setup();
    EXPECT_EQ(thread_method.get_count(), 0);
    Utils::sleep(1000);
    EXPECT_EQ(thread_method.get_count(), 0);
    dedicated_thread.start();
    Utils::sleep(1000);
    EXPECT_EQ(thread_method.get_count(), 1);
    Utils::sleep(1000);
    EXPECT_EQ(thread_method.get_count(), 1);
    thread_method.set_cycle_flag(true);
    Utils::sleep(1000);
    EXPECT_EQ(thread_method.get_count(), 2);
    dedicated_thread.stop();
    Utils::sleep(1000);
    EXPECT_EQ(thread_method.get_count(), 2);
}

TEST(ProcessorTest, thread_pool) {
    mutex count_mutex;
    int count = 0;

    auto job_inc = [&count, &count_mutex]() {
        count_mutex.lock();
        count++;
        count_mutex.unlock();
        Utils::sleep(Utils::uint_rand(10));
    };

    auto job_dec = [&count, &count_mutex]() {
        count_mutex.lock();
        count--;
        count_mutex.unlock();
        Utils::sleep(Utils::uint_rand(10));
    };

    ThreadPool* pool;
    for (unsigned int n_threads : {1, 2, 4, 10, 100}) {
        for (int n_inc : {100, 200, 300}) {
            for (int n_dec : {50, 150, 250}) {
                count = 0;
                pool = new ThreadPool("Pool:" + std::to_string(n_threads), n_threads);
                pool->setup();
                pool->start();
                int count_inc = 0;
                int count_dec = 0;
                for (int i = 0; i < (n_inc + n_dec); i++) {
                    if (count_inc == n_inc) {
                        pool->enqueue(job_dec);
                        count_dec++;
                    } else if (count_dec == n_dec) {
                        pool->enqueue(job_inc);
                        count_inc++;
                    } else {
                        if (Utils::flip_coin(n_inc / ((double) n_inc + n_dec))) {
                            pool->enqueue(job_inc);
                            count_inc++;
                        } else {
                            pool->enqueue(job_dec);
                            count_dec++;
                        }
                    }
                }
                EXPECT_EQ(count_inc, n_inc);
                EXPECT_EQ(count_dec, n_dec);
                pool->wait();
                LOG_INFO("n_threads: " << n_threads << " n_inc: " << n_inc << " n_dec: " << n_dec);
                EXPECT_EQ(count, (n_inc - n_dec));
                pool->stop();
                delete (pool);
            }
        }
    }
}
