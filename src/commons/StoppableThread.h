#pragma once

#include <mutex>
#include <string>
#include <thread>

#include "Stoppable.h"

using namespace std;

namespace commons {

/**
 * Wrapper to thread objects to externelly (outside the thread method/function) make it stop.
 * As side effect, we can also check if the thread has already been stopped.
 *
 * To use StoppableThread:
 *
 * (1) Create a shared_ptr<StoppableThread> object passing an ID string (this id is used in logging
 * messages). The idea is that this ID is unique but this is not actually required in the scope of
 *     StoppableThread. Uniqueness, however, is useful in scenarios where we have multiple
 *     threads running the same method/function and we need to distiguinsh among them (either in
 *     the logs or in the code that controls the threads).
 * (2) Make the thread method/function (i.e. the method/function which will be executed by the
 *     thread) expect a shared_ptr<StoppableThread> object as argument. Inside the thread
 *     method/function, use stopped() to control if the thread is supposed to abort execution.
 *     It's OK if the method/function reaches the end without stopped() being trigered. The idea
 *     of the Stoppable interface is to allow "aborting" behavior.
 * (3) After creating the StoppedThread object as described in (1), call attach() passing the std::thread
 *     object.
 * (4) Outside the thread method/function, use stop() to make the thread stop, join and be deleted.
 * Joining and deletion are optional. Use stop(false) to skip them.
 */
class StoppableThread : public Stoppable {
   public:
    /**
     * Constructor.
     *
     * @param id ID of the wrapped thread. It's desirable (but not required) that this id is unique per
     * thread in the scope of the caller.
     */
    StoppableThread(const string& id);

    /**
     * Destructor.
     */
    ~StoppableThread();

    /**
     * Attach this StoppableThread object to a std::thread object
     */
    void attach(thread* thread_object);

    /**
     * Stop attached thread.
     *
     * No action (other than flipping a flag inside this StoppableThread object) is actually taken
     * in order to stop the thread. The "stopping" process is supposed to happen inside the thread
     * method/function which is supposed to check for this flag to know if it should abort
     * execution.
     *
     * @param join_thread If true, the attached thread will be joined (by calling thread::join())
     * and deleted. Otherwise, none of these actions are taken.
     */
    void stop(bool join_thread);

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
    void stop();

    /**
     * Returns true iff the "stop" flag has be set.
     *
     * @return true iff the "stop" flag has be set.
     */
    bool stopped();

    /**
     * Getter for the ID of this StoppableThread.
     *
     * @return The ID of this StoppableThread.
     */
    string get_id();

   private:
    bool stop_flag;
    string id;
    mutex stop_flag_mutex;
    thread* thread_object;
};

}  // namespace commons
