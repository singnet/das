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

    /**
     * @brief Set the timeout
     * @param timeout Timeout in seconds
     */
    void set_timeout(unsigned int timeout);

    /**
     * @brief Get the timeout
     * @return unsigned int
     */
    unsigned int get_timeout();

    /**
     * @brief Get the repeat count
     * @return unsigned int
     */
    unsigned int get_repeat();

    /**
     * @brief Set the repeat count
     * @param repeat Repeat count
     */
    void set_repeat(unsigned int repeat);
    /**
     * @brief Set the max results for link creation agent
     * @param lca_max_results Max results for link creation agent
     */
    void set_lca_max_results(unsigned long lca_max_results);

    /**
     * @brief Get the max results for link creation agent
     * @return unsigned long
     */
    unsigned long get_lca_max_results();

    /**
     * @brief Get the max repeats for link creation agent
     * @return unsigned int
     */
    unsigned int get_lca_max_repeats();

    /**
     * @brief Set the max repeats for link creation agent
     * @param lca_max_repeats Max repeats for link creation agent
     */
    void set_lca_max_repeats(unsigned int lca_max_repeats);

    /**
     * @brief Get the update attention broker flag for link creation agent
     * @return bool
     */
    bool get_lca_update_attention_broker();

    /**
     * @brief Set the update attention broker flag for link creation agent
     * @param lca_update_attention_broker Update attention broker flag for link creation agent
     */
    void set_lca_update_attention_broker(bool lca_update_attention_broker);

   protected:
    string first_handle;
    string second_handle;
    int max_proof_length;
    string context;
    string inference_request_id;
    string max_answers;
    string update_attention_broker;
    unsigned long long timeout = 24 * 60 * 60;  // Default timeout is 24 hours
    unsigned int repeat = 5;
    unsigned long lca_max_results = 100;  // Default max results for LCA
    unsigned int lca_max_repeats = 1;      // Default max repeats for LCA
    bool lca_update_attention_broker = false;  // Default update attention broker for LCA
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
