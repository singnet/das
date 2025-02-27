/**
 * @file inference_request.h
 *
 */

#pragma once

#include <string>
#include <vector>

namespace inference_agent {

class InferenceRequest {
   public:
    /**
     * @brief Construct a new Inference Request object
     */
    InferenceRequest(std::string first_handle, std::string second_handle, int max_proof_length);
    ~InferenceRequest();

    /**
     * @brief Tokenize the inference request
     *
     * @return std::vector<std::string>
     */
    virtual std::vector<std::string> tokenize();
    /**
     * @brief Untokenize the inference request
     *
     * @return std::vector<std::string>
     */
    virtual std::vector<std::string> untokenize();
    /**
     * @brief Generate a unique id
     *
     * @return std::string
     */

    /**
     * @brief Query to send to link creation agent
     *
     * @return std::vector<std::string>
     */
    virtual std::vector<std::string> query();

    /**
     * @brief Get the id of the inference request
     *
     * @return std::string
     */
    virtual std::string get_id();

    /**
     * @brief Get the type of the inference request
     *
     * @return std::string
     */
    virtual std::string get_type();

    /**
     * @brief Get the max proof length of the inference request
     *
     * @return std::string
     */
    virtual std::string get_max_proof_length();

    /**
     * @brief Get the distributed inference control request of the inference request
     *
     * @return std::vector<std::string>
     */
    virtual std::vector<std::string> get_distributed_inference_control_request();

   protected:
    std::string first_handle;
    std::string second_handle;
    int max_proof_length;
};

class ProofOfImplicationOrEquivalence : public InferenceRequest {
   public:
    ProofOfImplicationOrEquivalence(std::string first_handle,
                                    std::string second_handle,
                                    int max_proof_length);
    ~ProofOfImplicationOrEquivalence();

    std::vector<std::string> tokenize() override;
    std::vector<std::string> untokenize() override;
    std::vector<std::string> query() override;
    std::vector<std::string> patterns_link_template();
    std::string get_id() override;
    std::string get_type() override;
    std::string get_max_proof_length() override;
    std::vector<std::string> get_distributed_inference_control_request() override;
};

class ProofOfImplication : public InferenceRequest {
   public:
    ProofOfImplication(std::string first_handle, std::string second_handle, int max_proof_length);
    ~ProofOfImplication();

    std::vector<std::string> tokenize() override;
    std::vector<std::string> untokenize() override;
    std::vector<std::string> query() override;
    std::string get_id() override;
    std::string get_type() override;
    std::string get_max_proof_length() override;
    std::vector<std::string> get_distributed_inference_control_request() override;
   private:
    const std::string IMPLICATION_DEDUCTION_PROCESSOR = "IMPLICATION_DEDUCTION";
};

class ProofOfEquivalence : public InferenceRequest {
   public:
    ProofOfEquivalence(std::string first_handle, std::string second_handle, int max_proof_length);
    ~ProofOfEquivalence();

    std::vector<std::string> tokenize() override;
    std::vector<std::string> untokenize() override;
    std::vector<std::string> query() override;
    std::string get_id() override;
    std::string get_type() override;
    std::string get_max_proof_length() override;
    std::vector<std::string> get_distributed_inference_control_request() override;
   private:
    const std::string EQUIVALENCE_DEDUCTION_PROCESSOR = "EQUIVALENCE_DEDUCTION";
};

}  // namespace inference_agent