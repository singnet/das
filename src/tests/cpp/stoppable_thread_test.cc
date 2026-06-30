#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <thread>

#include "StoppableThread.h"
#include "Utils.h"
#include "gtest/gtest.h"

using namespace std;
using namespace commons;

// Reproduces the self-reaping pattern used by PatternMatchingQueryProcessor and
// QueryEvolutionProcessor: a pool of worker threads where each worker, when it finishes, detaches
// itself and removes its own entry from a shared map. This is the "burst-then-idle" case: after a
// burst of workers completes, the map must drain to empty WITHOUT any external/non-worker reaping
// trigger.
TEST(StoppableThread, self_reap_burst_then_idle) {
    map<string, shared_ptr<StoppableThread>> threads;
    mutex threads_mutex;
    atomic<int> completed{0};

    const int burst = 16;
    for (int i = 0; i < burst; i++) {
        string id = "worker_" + std::to_string(i);
        lock_guard<mutex> guard(threads_mutex);
        auto stoppable_thread = make_shared<StoppableThread>(id);
        stoppable_thread->attach(new thread(
            [&threads, &threads_mutex, &completed](shared_ptr<StoppableThread> monitor) {
                Utils::sleep(10);  // simulate a short unit of work
                completed++;
                // Self-reap (mirrors the processors): a thread can't join itself, so detach and
                // drop its own entry. completed++ happens before the erase, so once the map is
                // empty every worker is guaranteed to have finished its work.
                lock_guard<mutex> guard(threads_mutex);
                monitor->detach();
                threads.erase(monitor->get_id());
            },
            stoppable_thread));
        threads[id] = stoppable_thread;
    }

    // Bounded wait for the burst to drain, issuing no further "commands".
    for (int i = 0; i < 500; i++) {
        {
            lock_guard<mutex> guard(threads_mutex);
            if (threads.empty()) {
                break;
            }
        }
        Utils::sleep(10);
    }

    lock_guard<mutex> guard(threads_mutex);
    EXPECT_EQ(completed.load(), burst);
    EXPECT_TRUE(threads.empty());
}

// detach() must make the thread non-joinable so that a subsequent stop() (e.g. from the
// StoppableThread destructor) is a safe no-op rather than a join-on-detached error.
TEST(StoppableThread, stop_after_detach_is_noop) {
    auto stoppable_thread = make_shared<StoppableThread>("detach_then_stop");
    atomic<bool> ran{false};
    stoppable_thread->attach(new thread(
        [&ran](shared_ptr<StoppableThread> monitor) {
            ran = true;
            while (!monitor->stopped()) {
                Utils::sleep(5);
            }
        },
        stoppable_thread));

    while (!ran.load()) {
        Utils::sleep(5);
    }
    stoppable_thread->detach();                 // releases the OS thread; also flips the stop flag
    EXPECT_TRUE(stoppable_thread->stopped());
    EXPECT_NO_THROW(stoppable_thread->stop());  // must not join/double-free a detached thread
}
