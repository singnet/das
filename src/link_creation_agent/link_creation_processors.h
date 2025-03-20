/**
 * @file link_creation_processors.h
 */

#pragma once
#include <string>
#include <stdexcept>

using namespace std;
namespace link_creation_agent{
enum class ProcessorType {
    PROOF_OF_IMPLICATION,
    PROOF_OF_EQUIVALENCE,
};

class LinkCreationProcessor {
   public:
    static ProcessorType get_processor_type(string processor) {
        if (processor == "PROOF_OF_IMPLICATION") {
            return ProcessorType::PROOF_OF_IMPLICATION;
        } else if (processor == "PROOF_OF_EQUIVALENCE") {
            return ProcessorType::PROOF_OF_EQUIVALENCE;
        } else {
            throw runtime_error("Invalid processor type");
        }
    }
};

} // namespace link_creation_agent