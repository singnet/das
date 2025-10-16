#pragma once

#include <memory>

#include "BusCommandProxy.h"
#include "BusCommandProcessor.h"

using namespace std;
using namespace service_bus;
using namespace commons;

namespace atomdb_broker {

class AtomDBBrokerProcessor : public BusCommandProcessor {
   public:
    AtomDBBrokerProcessor();
    ~AtomDBBrokerProcessor();
    virtual shared_ptr<BusCommandProxy> factory_empty_proxy() override;
    virtual void run_command(shared_ptr<BusCommandProxy> proxy) override;
};

}  // namespace atomdb_broker
