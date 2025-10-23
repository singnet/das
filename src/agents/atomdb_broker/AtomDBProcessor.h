#pragma once

#include <map>
#include <memory>
#include <thread>

#include "AtomDBProxy.h"
#include "BusCommandProcessor.h"
#include "BusCommandProxy.h"
#include "StoppableThread.h"

using namespace std;
using namespace service_bus;
using namespace commons;

namespace atomdb_broker {

class AtomDBProcessor : public BusCommandProcessor {
   public:
    AtomDBProcessor();
    ~AtomDBProcessor();
    virtual shared_ptr<BusCommandProxy> factory_empty_proxy() override;
    virtual void run_command(shared_ptr<BusCommandProxy> proxy) override;

   private:
    void thread_process_one_query(shared_ptr<StoppableThread>, shared_ptr<AtomDBProxy> proxy);
    map<string, shared_ptr<StoppableThread>> query_threads;
    mutex query_threads_mutex;
};

}  // namespace atomdb_broker
