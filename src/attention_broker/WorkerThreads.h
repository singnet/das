#ifndef _ATTENTION_BROKER_SERVER_WORKERTHREADS_H
#define _ATTENTION_BROKER_SERVER_WORKERTHREADS_H

#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <mutex>

#include "SharedQueue.h"

using namespace std;
using namespace commons;

namespace attention_broker_server {

/**
 * Used in AttentionBrokerServer to keep track of worker threads.
 *
 * WorkerThreads provides an abstraction to actual threads creation and shutdown.
 */
class WorkerThreads {

public:

    /**
     * Constructor.
     *
     * Start n worker threads (n is a parameter defined in AttentionBrokerServer) and
     * keep then running getting requests from the queues which have been passed as
     * parameters.
     *
     * Working threads can process any type of requests. The policy of which request
     * queue a worker thread will read from next is determined by RequestSelector.
     */
    WorkerThreads(SharedQueue *stimulus, SharedQueue *correlation);
    ~WorkerThreads(); /// Destructor.

    /**
     * Gracefully and synchronously stop all threads.
     *
     * Sets a flag which is check by each thread when the requests queue are empty. It means
     * that both requests queues will be processed before the threads actually stop. When
     * both requests queues are empty, threads return and are destroyed. This method will wait
     * for all threads to finish before returning.
     */
    void graceful_stop();

private:

    unsigned int threads_count;
    vector<thread *> threads;
    bool stop_flag = false;
    SharedQueue *stimulus_requests;
    SharedQueue *correlation_requests;
    mutex stop_flag_mutex;

    void worker_thread(
        unsigned int thread_id, 
        SharedQueue *stimulus_requests, 
        SharedQueue *correlation_requests);
};

} // namespace attention_broker_server

#endif // _ATTENTION_BROKER_SERVER_WORKERTHREADS_H
