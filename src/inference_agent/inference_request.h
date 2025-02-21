/**
 * @file inference_request.h
 *
 */

#pragma once
#include <uuid/uuid.h>

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
    virtual std::vector<std::string> tokenize() = 0;
    /**
     * @brief Untokenize the inference request
     *
     * @return std::vector<std::string>
     */
    virtual std::vector<std::string> untokenize() = 0;
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
    virtual std::vector<std::string> query() = 0;

    /**
     * @brief Get the id of the inference request
     *
     * @return std::string
     */
    virtual std::string get_id() = 0;

    /**
     * @brief Get the type of the inference request
     *
     * @return std::string
     */
    virtual std::string get_type() = 0;

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


   private:
    const std::string EQUIVALENCE_DEDUCTION_PROCESSOR = "EQUIVALENCE_DEDUCTION";
};

}  // namespace inference_agent