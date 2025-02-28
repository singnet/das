#include "inference_request.h"

#include "link_create_template.h"
#include "memory"

using namespace std;
using namespace inference_agent;
using namespace link_creation_agent;

InferenceRequest::InferenceRequest(std::string first_handle,
                                   std::string second_handle,
                                   int max_proof_length) {
    this->first_handle = first_handle;
    this->second_handle = second_handle;
    this->max_proof_length = max_proof_length;
}

InferenceRequest::~InferenceRequest() {}

std::vector<std::string> InferenceRequest::tokenize() { return {}; }

std::vector<std::string> InferenceRequest::untokenize() { return {}; }

std::vector<std::string> InferenceRequest::query() { return {}; }

std::string InferenceRequest::get_id() { return ""; }

std::string InferenceRequest::get_type() { return "INFERENCE_REQUEST"; }

std::string InferenceRequest::get_max_proof_length() { return std::to_string(max_proof_length); }

std::vector<std::string> InferenceRequest::get_distributed_inference_control_request() { return {}; }

ProofOfImplicationOrEquivalence::ProofOfImplicationOrEquivalence(std::string first_handle,
                                                                 std::string second_handle,
                                                                 int max_proof_length)
    : InferenceRequest(first_handle, second_handle, max_proof_length) {}

ProofOfImplicationOrEquivalence::~ProofOfImplicationOrEquivalence() {}

std::vector<std::string> ProofOfImplicationOrEquivalence::tokenize() { return {}; }

std::vector<std::string> ProofOfImplicationOrEquivalence::untokenize() { return {}; }

std::vector<std::string> ProofOfImplicationOrEquivalence::query() {
    std::vector<std::string> tokens = {"LINK_TEMPLATE",
                                       "Expression",
                                       "3",
                                       "NODE",
                                       "Symbol",
                                       "EVALUATION",
                                       "LINK_TEMPLATE",
                                       "Expression",
                                       "2",
                                       "NODE",
                                       "Symbol",
                                       "PREDICATE",
                                       "VARIABLE",
                                       "P",
                                       "LINK_TEMPLATE",
                                       "Expression",
                                       "2",
                                       "NODE",
                                       "Symbol",
                                       "CONCEPT",
                                       "VARIABLE",
                                       "C"};
    return tokens;
}

std::vector<std::string> ProofOfImplicationOrEquivalence::patterns_link_template() {
    // SATISFYING_SET
    LinkCreateTemplate satisfying_set_link_template = LinkCreateTemplate("Expression");
    // Node("Symbol", "SATISFYING_SET");
    Node evaluation_node;
    evaluation_node.type = "Symbol";
    evaluation_node.value = "SATISFYING_SET";
    // Variable("P");
    Variable evaluation_variable;
    evaluation_variable.name = "P";
    satisfying_set_link_template.add_target(evaluation_node);
    satisfying_set_link_template.add_target(evaluation_variable);
    auto custom_field = CustomField("truth_value");
    custom_field.add_field("strength", "1.0");
    custom_field.add_field("confidence", "1.0");
    satisfying_set_link_template.add_custom_field(custom_field);

    // PATTERNS
    LinkCreateTemplate patterns_link_template = LinkCreateTemplate("Expression");
    // Node("Symbol", "PATTERNS");
    Node patterns_node;
    patterns_node.type = "Symbol";
    patterns_node.value = "PATTERNS";
    // Variable("C");
    Variable patterns_variable;
    patterns_variable.name = "C";
    patterns_link_template.add_target(patterns_node);
    patterns_link_template.add_target(patterns_variable);
    auto patterns_custom_field = CustomField("truth_value");
    patterns_custom_field.add_field("strength", "1.0");
    patterns_custom_field.add_field("confidence", "1.0");
    patterns_link_template.add_custom_field(patterns_custom_field);

    std::vector<std::string> tokens;
    tokens.push_back("LIST");
    tokens.push_back("2");
    for (auto token : satisfying_set_link_template.tokenize()) {
        tokens.push_back(token);
    }
    for (auto token : patterns_link_template.tokenize()) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string ProofOfImplicationOrEquivalence::get_type() { return "PROOF_OF_IMPLICATION_OR_EQUIVALENCE"; }

std::string ProofOfImplicationOrEquivalence::get_id() { return ""; }

std::string ProofOfImplicationOrEquivalence::get_max_proof_length() {
    return std::to_string(max_proof_length);
}

std::vector<std::string> ProofOfImplicationOrEquivalence::get_distributed_inference_control_request() {
    std::vector<std::string> tokens = {
        "CHAIN",       first_handle,    second_handle, std::to_string(max_proof_length),
        "OR",          "LINK_TEMPLATE", "Expression",  "3",
        "NODE",        "Symbol",        "IMPLICATION", first_handle,
        second_handle, "LINK_TEMPLATE", "Expression",  "3",
        "NODE",        "Symbol",        "EQUIVALENCE", first_handle,
        second_handle};
    return tokens;
}