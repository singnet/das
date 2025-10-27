#pragma once

#include "BaseQueryProxy.h"

using namespace std;
using namespace agents;
namespace inference_agent {

class InferenceProxy : public BaseQueryProxy {
   public:
    // Controls Inference process
    /** Inference request timeout */
    static string INFERENCE_REQUEST_TIMEOUT;
    /** Repeats the inference process N times */
    static string REPEAT_COUNT;
    // Control LCA process
    /** Maximum number of answers to return for each query and LCA queries */
    static string MAX_RESULTS;
    /** Controls if LCA should update the attention broker for each query */
    static string UPDATE_ATTENTION_BROKER;
    /** Run full evaluation query */
    static string RUN_FULL_EVALUATION_QUERY;  // TODO remove

    InferenceProxy();
    InferenceProxy(const vector<string>& tokens);
    virtual ~InferenceProxy();

    /**
     * Pack the command line arguments into the args vector.
     */
    void pack_command_line_args();

    /**
     * Set default parameters for the inference agent.
     */
    void set_default_parameters();

    /**
     * Set a parameter for the inference agent.
     */
    void set_parameter(const string& key, const PropertyValue& value);

   private:
    mutex api_mutex;
};

}  // namespace inference_agent
