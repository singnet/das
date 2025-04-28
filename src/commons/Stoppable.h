#pragma once

using namespace std;

namespace commons {

/**
 * Abstract interface to mark elements that can be stopped and monitored for its
 * "stopped" status.
 */
class Stoppable {
   public:
    virtual void stop() = 0;
    virtual bool stopped() = 0;

    Stoppable() {}
    ~Stoppable() {}
};

}  // namespace commons
