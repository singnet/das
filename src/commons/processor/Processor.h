#pragma once

#include <mutex>

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
    virtual void setup();
    virtual void start();
    virtual void stop();
    bool is_setup();
    bool is_finished();
    string to_string();

   private:
    void check_state(const string& action, State state);

    State current_state;
    string id;
    mutex api_mutex;
};

}  // namespace processor
