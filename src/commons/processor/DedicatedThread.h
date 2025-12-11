#pragma once

#include <thread>
#include <mutex>
#include "Processor.h"

using namespace std;

namespace processor {

class ThreadMethod {
    public:
    virtual bool thread_one_step() = 0;
};

/**
 *
 */
class DedicatedThread : public Processor {

public:

    DedicatedThread(const string& id, ThreadMethod* job);
    virtual ~DedicatedThread();

    virtual void setup();
    virtual void start();
    virtual void stop();

private:

    void thread_method();
    bool inline started();
    bool inline stopped();

    ThreadMethod* job;
    thread* thread_object;
    bool start_flag;
    bool stop_flag;
    mutex api_mutex;
};

} // namespace processor
