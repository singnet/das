/**
 * @file inference_request_validator.h
 * @brief Validates inference requests
 */

#pragma once

#include <regex>
#include <string>
#include <vector>

class RequestValidator {
   public:
    RequestValidator(const string& validator_regex) { this->validator_regex = validator_regex; }
    bool validate(vector<string>& request) {
        if (request.size() != count_tokens(validator_regex)) {
            return false;
        }
        std::ostringstream joined_request;
        for (auto& token : request) {
            joined_request << token << " ";
        }
        string joined_request_str = joined_request.str();
        joined_request_str.pop_back();
        return regex_match(joined_request_str, regex(validator_regex));
    }

   protected:
    string validator_regex;
    int count_tokens(const std::string& regex) {
        std::regex tokenRegex(R"(\S+)");  // Match non-space sequences
        std::sregex_iterator begin(regex.begin(), regex.end(), tokenRegex);
        std::sregex_iterator end;
        return std::distance(begin, end);
    }
};

using namespace std;
namespace inference_agent {

class InferenceRequestValidator: public RequestValidator {
   public:
    InferenceRequestValidator() : RequestValidator(validator_regex) {}

   private:
    const string validator_regex =
        "^(PROOF_OF_IMPLICATION_OR_EQUIVALENCE|PROOF_OF_IMPLICATION|PROOF_OF_EQUIVALENCE) [a-zA-Z0-9_]+ "
        "[a-zA-Z0-9_]+ [0-9]+";
};

}  // namespace inference_agent
