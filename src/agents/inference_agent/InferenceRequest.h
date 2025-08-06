/**
 * @file InferenceRequest.h
 * @brief InferenceRequest class is a helper
 * class to create inference requests to be sent to the link creation agent
 * and distributed inference control agent.
 */

#pragma once

#include <string>
#include <vector>

using namespace std;

namespace inference_agent {

class InferenceRequest {
   public:
    /**
     * @brief Construct a new Inference Request object
     */
    InferenceRequest(string first_handle,
                     string second_handle,
                     int max_proof_length,
                     string context,
                     string max_answers = "1000000",
                     string update_attention_broker = "false");
    ~InferenceRequest();
    /**
     * @brief Query to send to link creation agent
     *
     * @return vector<string>
     */
    virtual vector<string> query();

    /**
     * @brief Get the id of the inference request
     *
     * @return string
     */
    string get_id();

    /**
     * @brief Set the id of the inference request
     *
     * @param inference_request_id
     */
    void set_id(string inference_request_id);

    /**
     * @brief Get the type of the inference request
     *
     * @return string
     */
    virtual string get_type();

    /**
     * @brief Get the max proof length of the inference request
     *
     * @return string
     */
    virtual string get_max_proof_length();

    /**
     * @brief Get the distributed inference control request of the inference request
     *
     * @return vector<string>
     */
    virtual vector<string> get_distributed_inference_control_request();

    /**
     * @brief Get the requests of the inference request
     */
    virtual vector<vector<string>> get_requests();

    /**
     * @brief Get the context
     */
    string get_context();

    void set_timeout(unsigned int timeout);
    unsigned int get_timeout();

   protected:
    string first_handle;
    string second_handle;
    int max_proof_length;
    string context;
    string inference_request_id;
    string max_answers;
    string update_attention_broker;
    unsigned long long timeout = 10 * 60;  // Default timeout is 10 minutes
};

class ProofOfImplicationOrEquivalence : public InferenceRequest {
   public:
    ProofOfImplicationOrEquivalence(string first_handle,
                                    string second_handle,
                                    int max_proof_length,
                                    string context);
    ~ProofOfImplicationOrEquivalence();

    vector<string> query() override;
    vector<string> patterns_link_template();
    string get_type() override;
    vector<vector<string>> get_requests() override;
};

class ProofOfImplication : public InferenceRequest {
   public:
    ProofOfImplication(string first_handle, string second_handle, int max_proof_length, string context);
    ~ProofOfImplication();

    vector<string> query() override;
    string get_type() override;
    vector<vector<string>> get_requests() override;

   private:
    const string IMPLICATION_DEDUCTION_PROCESSOR = "IMPLICATION_DEDUCTION";
};

class ProofOfEquivalence : public InferenceRequest {
   public:
    ProofOfEquivalence(string first_handle, string second_handle, int max_proof_length, string context);
    ~ProofOfEquivalence();

    vector<string> query() override;
    string get_type() override;
    vector<vector<string>> get_requests() override;

   private:
    const string EQUIVALENCE_DEDUCTION_PROCESSOR = "EQUIVALENCE_DEDUCTION";
};

}  // namespace inference_agent
