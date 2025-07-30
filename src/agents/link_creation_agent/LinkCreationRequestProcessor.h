#pragma once

#include "BusCommandProcessor.h"
#include "LinkCreationAgent.h"

using namespace std;
using namespace service_bus;

namespace link_creation_agent {
class LinkCreationRequestProcessor : public BusCommandProcessor {
   public:
    LinkCreationRequestProcessor(int request_interval,
                                 int thread_count,
                                 int default_timeout,
                                 string buffer_file_path,
                                 string metta_file_path,
                                 bool save_links_to_metta_file = true,
                                 bool save_links_to_db = false,
                                 bool reindex = true);
    ~LinkCreationRequestProcessor();
    virtual shared_ptr<BusCommandProxy> factory_empty_proxy() override;
    virtual void run_command(shared_ptr<BusCommandProxy> proxy) override;

   private:
    LinkCreationAgent* link_creation_agent;
    void process_link_create_request(shared_ptr<BusCommandProxy> proxy);
};

}  // namespace link_creation_agent
