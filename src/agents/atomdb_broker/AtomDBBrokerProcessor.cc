#include "AtomDBBrokerProcessor.h"

#include "ServiceBus.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"
#include "AtomDBBrokerProxy.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace atomdb_broker;

// -------------------------------------------------------------------------------------------------
// Constructor and destructor

AtomDBBrokerProcessor::AtomDBBrokerProcessor() : BusCommandProcessor({ServiceBus::ATOMDB}) {}

AtomDBBrokerProcessor::~AtomDBBrokerProcessor() {}

// -------------------------------------------------------------------------------------------------
// Public methods

shared_ptr<BusCommandProxy> AtomDBBrokerProcessor::factory_empty_proxy() {
    return make_shared<AtomDBBrokerProxy>();
}

void AtomDBBrokerProcessor::run_command(shared_ptr<BusCommandProxy> proxy) {
    auto atomdb_broker_proxy = dynamic_pointer_cast<AtomDBBrokerProxy>(proxy);
    atomdb_broker_proxy->untokenize(atomdb_broker_proxy->args);
    if (atomdb_broker_proxy->get_command() == ServiceBus::ATOMDB) {
        while (atomdb_broker_proxy->running()) {
            Utils::sleep();
        }
    } else {
        atomdb_broker_proxy->raise_error_on_peer("Invalid command: " + atomdb_broker_proxy->get_command());
    }
}

