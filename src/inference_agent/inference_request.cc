#include "inference_request.h"

#include "Utils.h"
#include "link_create_template.h"
#include "memory"

using namespace std;
using namespace inference_agent;
using namespace link_creation_agent;
using namespace commons;

InferenceRequest::InferenceRequest(std::string first_handle,
                                   std::string second_handle,
                                   int max_proof_length) {
    this->first_handle = first_handle;
    this->second_handle = second_handle;
    this->max_proof_length = max_proof_length;
}

InferenceRequest::~InferenceRequest() {}

std::vector<std::string>&& InferenceRequest::query() { return {}; }

std::string InferenceRequest::get_id() { return Utils::random_string(10); }

std::string InferenceRequest::get_type() { return "INFERENCE_REQUEST"; }

std::string InferenceRequest::get_max_proof_length() { return std::to_string(max_proof_length); }

std::vector<std::string>&& InferenceRequest::get_distributed_inference_control_request() {
    // clang-format off
    return move(std::vector<std::string>({
        "CHAIN",
            first_handle,    
            second_handle, 
            std::to_string(max_proof_length),
            "OR",          
                "LINK_TEMPLATE", "Expression",  "3",
                    "NODE", "Symbol", "IMPLICATION", 
                    first_handle,
                    second_handle, 
                "LINK_TEMPLATE", "Expression",  "3",
                    "NODE", "Symbol", "EQUIVALENCE", 
                    first_handle,
                    second_handle
    }));
    // clang-format on
}

std::vector<std::vector<std::string>>&& InferenceRequest::get_requests() {
    return move(std::vector<std::vector<std::string>>{});
}

ProofOfImplicationOrEquivalence::ProofOfImplicationOrEquivalence(std::string first_handle,
                                                                 std::string second_handle,
                                                                 int max_proof_length)
    : InferenceRequest(first_handle, second_handle, max_proof_length) {}

ProofOfImplicationOrEquivalence::~ProofOfImplicationOrEquivalence() {}

std::vector<std::string>&& ProofOfImplicationOrEquivalence::query() {
    // clang-format off
    return move(std::vector<std::string>({
        "LINK_TEMPLATE", "Expression", "3",
            "NODE", "Symbol", "EVALUATION",
            "LINK_TEMPLATE", "Expression", "2",
                "NODE", "Symbol", "PREDICATE",
                "VARIABLE", "P",
            "LINK_TEMPLATE", "Expression", "2",
                "NODE", "Symbol", "CONCEPT",
                "VARIABLE", "C"
    }));
    // clang-format on
}

std::vector<std::string>&& ProofOfImplicationOrEquivalence::patterns_link_template() {
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
    return move(tokens);
}

std::string ProofOfImplicationOrEquivalence::get_type() { return "PROOF_OF_IMPLICATION_OR_EQUIVALENCE"; }

std::vector<std::vector<std::string>>&& ProofOfImplicationOrEquivalence::get_requests() {
    std::vector<std::vector<std::string>> requests;
    // query + link creation template
    std::vector<std::string> query_and_link_creation_template(this->query());
    for (auto token : this->patterns_link_template()) {
        query_and_link_creation_template.push_back(token);
    }
    requests.push_back(query_and_link_creation_template);
    // proof of implication
    ProofOfImplication proof_of_implication(first_handle, second_handle, max_proof_length);
    for (auto request : proof_of_implication.get_requests()) {
        requests.push_back(request);
    }
    // proof of equivalence
    ProofOfEquivalence proof_of_equivalence(first_handle, second_handle, max_proof_length);
    for (auto request : proof_of_equivalence.get_requests()) {
        requests.push_back(request);
    }

    return move(requests);
}

ProofOfImplication::ProofOfImplication(std::string first_handle,
                                       std::string second_handle,
                                       int max_proof_length)
    : InferenceRequest(first_handle, second_handle, max_proof_length) {}

ProofOfImplication::~ProofOfImplication() {}

std::vector<std::string>&& ProofOfImplication::query() {
    // clang-format off
    return move(std::vector<std::string>({
        "AND", "3",
            "LINK_TEMPLATE", "Expression", "2",
                "NODE", "Symbol", "SATISFYING_SET",
                "VARIABLE", "P1",
            "LINK_TEMPLATE", "Expression", "2",
                "NODE", "Symbol", "SATISFYING_SET",
                "VARIABLE", "P2",
            "NOT",
                "OR",
                    "LINK_TEMPLATE", "Expression", "3",
                        "NODE", "Symbol", "IMPLICATION",
                        "LINK_TEMPLATE", "Expression", "3",
                            "NODE", "Symbol", "EVALUATION",
                            "VARIABLE", "P1",
                            "VARIABLE", "C",
                        "LINK_TEMPLATE", "Expression", "3",
                            "NODE", "Symbol", "EVALUATION",
                            "VARIABLE", "P2",
                            "VARIABLE", "C",
                    "LINK_TEMPLATE", "Expression", "3",
                        "NODE", "Symbol", "IMPLICATION",
                        "LINK_TEMPLATE", "Expression", "3",
                            "NODE", "Symbol", "EVALUATION",
                            "VARIABLE", "P2",
                            "VARIABLE", "C",
                        "LINK_TEMPLATE", "Expression", "3",
                            "NODE", "Symbol", "EVALUATION",
                            "VARIABLE", "P1",
                            "VARIABLE", "C"
    }));
    // clang-format on
}

std::string ProofOfImplication::get_type() { return "PROOF_OF_IMPLICATION"; }

std::vector<std::vector<std::string>>&& ProofOfImplication::get_requests() {
    std::vector<std::vector<std::string>> requests;
    auto query = this->query();
    query.push_back("IMPLICATION_DEDUCTION");  // processor
    requests.push_back(query);
    return move(requests);
}

ProofOfEquivalence::ProofOfEquivalence(std::string first_handle,
                                       std::string second_handle,
                                       int max_proof_length)
    : InferenceRequest(first_handle, second_handle, max_proof_length) {}

ProofOfEquivalence::~ProofOfEquivalence() {}

std::vector<std::string>&& ProofOfEquivalence::query() {
    // clang-format off
    return move(std::vector<std::string>({
        "AND", "3",
        "LINK_TEMPLATE", "Expression", "2",
            "NODE", "Symbol", "PATTERNS",
            "VARIABLE", "C1",
        "LINK_TEMPLATE", "Expression", "2",
            "NODE", "Symbol", "PATTERNS",
            "VARIABLE", "C2",
        "NOT",
            "OR",
                "LINK_TEMPLATE", "Expression", "3",
                    "NODE", "Symbol", "EQUIVALENCE",
                    "LINK_TEMPLATE", "Expression", "3",
                        "NODE", "Symbol", "EVALUATION",
                        "VARIABLE", "P",
                        "VARIABLE", "C1",
                    "LINK_TEMPLATE", "Expression", "3",
                        "NODE", "Symbol", "EVALUATION",
                        "VARIABLE", "P",
                        "VARIABLE", "C2",        
                "LINK_TEMPLATE", "Expression", "3",
                    "NODE", "Symbol", "EQUIVALENCE",
                    "LINK_TEMPLATE", "Expression", "3",
                        "NODE", "Symbol", "EVALUATION",
                        "VARIABLE", "P",
                        "VARIABLE", "C2",
                    "LINK_TEMPLATE", "Expression", "3",
                        "NODE", "Symbol", "EVALUATION",
                        "VARIABLE", "P",
                        "VARIABLE", "C1"
    }));
    // clang-format on
}

std::string ProofOfEquivalence::get_type() { return "PROOF_OF_EQUIVALENCE"; }

std::vector<std::vector<std::string>>&& ProofOfEquivalence::get_requests() {
    std::vector<std::vector<std::string>> requests;
    auto query = this->query();
    query.push_back("EQUIVALENCE_DEDUCTION");  // processor
    requests.push_back(query);
    return move(requests);
}
