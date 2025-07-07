#pragma once

#include "BusCommandProcessor.h"
#include "InferenceAgent.h"

using namespace std;
using namespace service_bus;

namespace inference_agent {
class InferenceProcessor : public BusCommandProcessor {
   public:
    InferenceProcessor();
    ~InferenceProcessor();

    virtual shared_ptr<BusCommandProxy> factory_empty_proxy() override;
    virtual void run_command(shared_ptr<BusCommandProxy> proxy) override;

   private:
    InferenceAgent* inference_agent;
    void process_inference_request(shared_ptr<BusCommandProxy> proxy);
};

}  // namespace inference_agent