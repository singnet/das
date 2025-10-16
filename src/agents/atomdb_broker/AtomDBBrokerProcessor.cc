#include "AtomDBBrokerProcessor.h"

#include "ServiceBus.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"
#include "AtomDBBrokerProxy.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace atomdb_broker;

// -------------------------------------------------------------------------------------------------
// Constructors and destructors

AtomDBBrokerProcessor::AtomDBBrokerProcessor() : BusCommandProcessor({ServiceBus::ATOMDB}) {}

AtomDBBrokerProcessor::~AtomDBBrokerProcessor() {}

// -------------------------------------------------------------------------------------------------
// Public methods

shared_ptr<BusCommandProxy> AtomDBBrokerProcessor::factory_empty_proxy() {
    return make_shared<AtomDBBrokerProxy>();
}

void AtomDBBrokerProcessor::run_command(shared_ptr<BusCommandProxy> proxy) {
    LOG_INFO("[[ AtomDBBrokerProcessor::run_command - 1 ]]");
    auto atomdb_broker_proxy = dynamic_pointer_cast<AtomDBBrokerProxy>(proxy);
    LOG_INFO("[[ AtomDBBrokerProcessor::run_command - 2 ]]");
    atomdb_broker_proxy->untokenize(atomdb_broker_proxy->args);
    LOG_INFO("[[ AtomDBBrokerProcessor::run_command - 3 ]]");
    if (atomdb_broker_proxy->get_command() == ServiceBus::ATOMDB) {
        LOG_INFO("[[ AtomDBBrokerProcessor::run_command - 4 ]]");
        while (atomdb_broker_proxy->running()) {
            Utils::sleep();
        }
        LOG_INFO("[[ AtomDBBrokerProcessor::run_command - 5 ]]");
    } else {
        atomdb_broker_proxy->raise_error_on_peer("Invalid command: " + atomdb_broker_proxy->get_command());
    }
    LOG_INFO("[[ AtomDBBrokerProcessor::run_command - 6 ]]");
}

