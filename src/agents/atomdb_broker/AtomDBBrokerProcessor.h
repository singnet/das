#pragma once

#include "BusCommandProcessor.h"
#include "AtomDBBrokerProxy.h"
#include "StoppableThread.h"

using namespace std;
using namespace service_bus;

namespace atomdb_broker {

class AtomDBBrokerProcessor : public BusCommandProcessor {
   public:
    AtomDBBrokerProcessor();
    ~AtomDBBrokerProcessor();
    virtual shared_ptr<BusCommandProxy> factory_empty_proxy() override;
    virtual void run_command(shared_ptr<BusCommandProxy> proxy) override;
   
   private:
    void thread_process_one_query(shared_ptr<StoppableThread> monitor, shared_ptr<AtomDBBrokerProxy> proxy);
    shared_ptr<AtomDBBrokerProxy> proxy;

    map<string, shared_ptr<StoppableThread>> query_threads;
    mutex query_threads_mutex;
};

}  // namespace atomdb_broker
