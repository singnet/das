#pragma once

#include <thread>
#include <mutex>
#include <string>

#include "Stoppable.h"

using namespace std;

namespace commons {

/**
 *
 */
class StoppableThread : public Stoppable {

public:

    StoppableThread(const string &id);
    ~StoppableThread();

    void attach(thread *thread_object);
    void stop(bool join_thread);
    void stop();
    bool stopped();
    string get_id();

private:

    bool stop_flag;
    string id;
    mutex stop_flag_mutex;
    thread *thread_object;
};

} // namespace commons
