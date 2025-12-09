#pragma once

#include <memory>
#include <mutex>
#include <vector>

using namespace std;

namespace processor {

/**
 *
 */
class Processor {
   public:
    enum State { UNDEFINED = 0, WAITING_SETUP, WAITING_START, WAITING_STOP, FINISHED };

    Processor(const string& id);
    ~Processor();

    static void bind_subprocessor(shared_ptr<Processor> root, shared_ptr<Processor> child);

    virtual void setup();
    virtual void start();
    virtual void stop();

    virtual void add_subprocessor(shared_ptr<Processor> other);
    virtual void set_parent(shared_ptr<Processor> other);

    bool is_setup();
    bool is_running();
    bool is_finished();
    string to_string();

   private:
    void check_state(const string& action, State state);

    State current_state;
    string id;
    vector<shared_ptr<Processor>> subprocessors;
    shared_ptr<Processor> parent_processor;
    mutex api_mutex;
};

}  // namespace processor
