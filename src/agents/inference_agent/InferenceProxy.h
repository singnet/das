#pragma once

#include "BaseQueryProxy.h"

using namespace std;
using namespace agents;
namespace inference_agent {

class InferenceProxy : public BaseQueryProxy {
   public:
    // struct Parameters {
    //     static const string UPDATE_ATTENTION_BROKER_FLAG;  // Interval for inference requests
    //     static const string MAX_ANSWERS;   // Timeout for inference requests
    // };
    static string INFERENCE_REQUEST_TIMEOUT;
    static string UPDATE_ATTENTION_BROKER_FLAG;  // Interval for inference requests
    static string REPEAT_REQUEST_NUMBER;         // Whether to repeat the request
    static string MAX_QUERY_ANSWERS_TO_PROCESS;  // Max number of query answers to process
    static string RUN_FULL_EVALUATION_QUERY;

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
