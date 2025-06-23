#pragma once


#include "BusCommandProcessor.h"
#include "LinkCreationAgent.h"


using namespace std;
using namespace service_bus;

namespace link_creation_agent {
    class LinkCreationRequestProcessor : public BusCommandProcessor {
    public:
        LinkCreationRequestProcessor();
        ~LinkCreationRequestProcessor();
        virtual shared_ptr<BusCommandProxy> factory_empty_proxy() override;
        virtual void run_command(shared_ptr<BusCommandProxy> proxy) override;
    private:
        LinkCreationAgent *link_creation_agent;
        void process_link_create_request(shared_ptr<BusCommandProxy> proxy);
        void abort_link_create_request(shared_ptr<BusCommandProxy> proxy);
    };
    
} // namespace link_creation_agent


