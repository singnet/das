#pragma once

#include <mutex>

using namespace std;

namespace commons {

/**
 * Abstract interface to mark elements that can be stopped and monitored for its
 * "stopped" status.
 */
class Stoppable {
   public:

    /**
     * Stop attached thread.
     *
     * No action (other than flipping a flag inside this StoppableThread object) is actually taken
     * in order to stop the thread. The "stopping" process is supposed to happen inside the thread
     * method/function which is supposed to check for this flag to know if it should abort
     * execution.
     *
     * The attached thread will be joined (by calling thread::join()) and deleted.
     */
    virtual void stop();

    /**
     * Returns true iff the "stop" flag has be set.
     *
     * @return true iff the "stop" flag has be set.
     */
    bool stopped();

    /**
     * Returns true iff the "stop" flag has be set. Otherwise, set `stop` to true and return false.
     *
     * @return true iff the "stop" flag has be set. Otherwise, set `stop` to true and return false.
     */
    bool check_and_set_stopped();

    Stoppable();
    ~Stoppable();

   private:
    bool stop_flag;
    mutex stop_flag_mutex;

};

}  // namespace commons
