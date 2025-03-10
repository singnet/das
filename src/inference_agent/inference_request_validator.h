/**
 * @file inference_request_validator.h
 * @brief Validates inference requests
 */

#pragma once

#include <regex>
#include <string>
#include <vector>
#include "RequestValidator.h"


using namespace std;
using namespace commons;
namespace inference_agent {

class InferenceRequestValidator : public RequestValidator {
   public:
    InferenceRequestValidator() { validator_regex = inference_request_validator_regex; }

   protected:
    const string inference_request_validator_regex =
        "^(PROOF_OF_IMPLICATION_OR_EQUIVALENCE|PROOF_OF_IMPLICATION|PROOF_OF_EQUIVALENCE) [a-zA-Z0-9_]+ "
        "[a-zA-Z0-9_]+ [0-9]+";
};

}  // namespace inference_agent
