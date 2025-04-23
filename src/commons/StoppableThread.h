#pragma once

#include <thread>
#include <mutex>
#include <string>

using namespace std;

namespace commons {

/**
 *
 */
class StoppableThread {

public:

    StoppableThread(const string &id);
    ~StoppableThread();

    void run(thread *thread_object);
    void stop();
    bool stopped();

private:

    bool stop_flag;
    string id;
    mutex stop_flag_mutex;
    thread *thread_object;
};

} // namespace commons
