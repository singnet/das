#include "inference_request.h"

using namespace std;
using namespace inference_agent;


InferenceRequest::InferenceRequest(std::string first_handle, std::string second_handle, int max_proof_length) {
    this->first_handle = first_handle;
    this->second_handle = second_handle;
    this->max_proof_length = max_proof_length;
}

InferenceRequest::~InferenceRequest() {}

std::vector<std::string> InferenceRequest::tokenize() {
    return {};
}

std::vector<std::string> InferenceRequest::untokenize() {
    return {};
}

std::vector<std::string> InferenceRequest::query() {
    return {};
}

std::string InferenceRequest::get_id() {
    return "";
}

std::string InferenceRequest::get_type() {
    return "INFERENCE_REQUEST";
}




ProofOfImplicationOrEquivalence::ProofOfImplicationOrEquivalence(std::string first_handle,
                                                                 std::string second_handle,
                                                                 int max_proof_length) : InferenceRequest(first_handle, second_handle, max_proof_length) {
}

ProofOfImplicationOrEquivalence::~ProofOfImplicationOrEquivalence() {}

std::vector<std::string> ProofOfImplicationOrEquivalence::tokenize() {
    return {};
} 

std::vector<std::string> ProofOfImplicationOrEquivalence::untokenize() {
    return {};
}

std::vector<std::string> ProofOfImplicationOrEquivalence::query() {
    return {};
}

std::vector<std::string> ProofOfImplicationOrEquivalence::patterns_link_template() {
    return {};
}

 

std::string ProofOfImplicationOrEquivalence::get_type() {
    return "PROOF_OF_IMPLICATION_OR_EQUIVALENCE";
}

std::string ProofOfImplicationOrEquivalence::get_id() {
    return "";
}






