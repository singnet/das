#pragma once

#include "gmock/gmock.h"
#include "ServiceBus.h"

using namespace std;
using namespace service_bus;

class MockServiceBus : public ServiceBus {
   public:
    MockServiceBus(const string& server_id, const string& peer_address)
        : ServiceBus(server_id, peer_address) {}

    MOCK_METHOD(void, register_processor, (shared_ptr<BusCommandProcessor> processor), (override));
    MOCK_METHOD(void, issue_bus_command, (shared_ptr<BusCommandProxy> command_proxy), (override));
};