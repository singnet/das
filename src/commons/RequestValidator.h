/**
 * @file RequestValidator.h
 */

#pragma once

#include <algorithm>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include "Utils.h"

using namespace std;

namespace commons {

class RequestValidator {
   public:
    RequestValidator() {}
    bool validate(vector<string>& request) {
        if (request.size() != count_tokens(validator_regex)) {
            return false;
        }
        string joined_request_str = Utils::join(request, ' ');
        return regex_match(joined_request_str, regex(validator_regex));
    }

   protected:
    string validator_regex;
    int count_tokens(const std::string& regex) {
        std::regex token_regex(R"(\S+)");  // Match non-space sequences
        std::sregex_iterator begin(regex.begin(), regex.end(), token_regex);
        std::sregex_iterator end;
        return std::distance(begin, end);
    }
};

}  // namespace commons
