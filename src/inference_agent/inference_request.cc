#include "inference_request.h"

#include <memory>

#include "Utils.h"
#include "link_create_template.h"

using namespace std;
using namespace inference_agent;
using namespace link_creation_agent;
using namespace commons;

InferenceRequest::InferenceRequest(std::string first_handle,
                                   std::string second_handle,
                                   int max_proof_length,
                                   std::string context) {
    this->first_handle = first_handle;
    this->second_handle = second_handle;
    this->max_proof_length = max_proof_length;
    this->context = context;
}

InferenceRequest::~InferenceRequest() {}

std::vector<std::string> InferenceRequest::query() { return {}; }

std::string InferenceRequest::get_id() { return inference_request_id; }

void InferenceRequest::set_id(std::string inference_request_id) {
    this->inference_request_id = inference_request_id;
}

std::string InferenceRequest::get_type() { return "INFERENCE_REQUEST"; }

std::string InferenceRequest::get_max_proof_length() { return std::to_string(max_proof_length); }

template <typename T>
static vector<vector<T>> product(const std::vector<T>& iterable, size_t repeat) {
    std::vector<std::vector<T>> result;
    std::vector<T> current(repeat, iterable[0]);
    result.push_back(current);

    for (size_t i = 0; i < repeat; i++) {
        std::vector<std::vector<T>> temp;
        for (const auto& item : iterable) {
            for (const auto& vec : result) {
                std::vector<T> new_vec = vec;
                new_vec[i] = item;
                temp.push_back(new_vec);
            }
        }
        result = std::move(temp);
    }

    return result;
}

static std::vector<std::string> inference_evolution_request_builder(std::string first_handle,
                                                                    std::string second_handle,
                                                                    int max_proof_length,
                                                                    int& counter) {
    // clang-format off
    std::vector<std::string> query_template = {
        "LINK_TEMPLATE", "Expression",  "3",
        "NODE", "Symbol", "_TYPE_", 
        "_FIRST_",
        "_SECOND_", };
    // clang-format on
    std::vector<std::string> commands = {"IMPLICATION", "EQUIVALENCE"};
    std::vector<std::string> request;
    if (max_proof_length == 1) {
        counter += commands.size();
        for (auto ttype : commands) {
            for (auto token : query_template) {
                if (token == "_TYPE_") {
                    request.push_back(ttype);
                } else if (token == "_FIRST_") {
                    request.push_back("HANDLE");
                    request.push_back(first_handle);
                } else if (token == "_SECOND_") {
                    request.push_back("HANDLE");
                    request.push_back(second_handle);
                } else {
                    request.push_back(token);
                }
            }
        }
    } else {
        std::vector<std::string> vars;
        vars.push_back(first_handle);
        for (int i = 1; i < max_proof_length; i++) {
            vars.push_back("V" + std::to_string(i));
        }
        vars.push_back(second_handle);
        auto commands_product = product<std::string>({commands}, vars.size() - 1);
        for (auto cp : commands_product) {
            counter++;
            request.push_back("AND");
            request.push_back(std::to_string(vars.size() - 1));
            for (int i = 1; i < vars.size(); i++) {
                for (auto token : query_template) {
                    if (token == "_TYPE_") {
                        request.push_back(cp[i - 1]);
                    } else if (token == "_FIRST_") {
                        request.push_back((vars[i - 1] == first_handle || vars[i - 1] == second_handle)
                                              ? "HANDLE"
                                              : "VARIABLE");
                        request.push_back(vars[i - 1]);
                    } else if (token == "_SECOND_") {
                        request.push_back((vars[i] == first_handle || vars[i] == second_handle)
                                              ? "HANDLE"
                                              : "VARIABLE");
                        request.push_back(vars[i]);
                    } else {
                        request.push_back(token);
                    }
                }
            }
        }

        // for (auto ttype : commands) {
        //     request.push_back("AND");
        //     request.push_back(std::to_string(vars.size() - 1));
        //     for (int i = 1; i < vars.size(); i++) {
        //         for (auto token : query_template) {
        //             if (token == "_TYPE_") {
        //                 request.push_back(ttype);
        //             } else if (token == "_FIRST_") {
        //                 request.push_back((vars[i - 1] == first_handle || vars[i - 1] ==
        //                 second_handle)? "HANDLE" : "VARIABLE"); request.push_back(vars[i - 1]);
        //             } else if (token == "_SECOND_") {
        //                 request.push_back((vars[i] == first_handle || vars[i] == second_handle)?
        //                 "HANDLE" : "VARIABLE"); request.push_back(vars[i]);
        //             } else {
        //                 request.push_back(token);
        //             }
        //         }
        //     }
        // }

        for (auto tkn : inference_evolution_request_builder(
                 first_handle, second_handle, max_proof_length - 1, counter)) {
            request.push_back(tkn);
        }
    }
    return request;
}

std::vector<std::string> InferenceRequest::get_distributed_inference_control_request() {
    std::vector<std::string> tokens;
    int size = 0;
    std::vector<std::string> request =
        inference_evolution_request_builder(first_handle, second_handle, max_proof_length, size);
    tokens.push_back(this->context);
    tokens.push_back("OR");
    tokens.push_back(std::to_string(size));
    for (auto token : request) {
        tokens.push_back(token);
    }
    return tokens;
}

std::vector<std::vector<std::string>> InferenceRequest::get_requests() { return {}; }

std::string InferenceRequest::get_context() { return context; }

ProofOfImplicationOrEquivalence::ProofOfImplicationOrEquivalence(std::string first_handle,
                                                                 std::string second_handle,
                                                                 int max_proof_length,
                                                                 std::string context)
    : InferenceRequest(first_handle, second_handle, max_proof_length, context) {}

ProofOfImplicationOrEquivalence::~ProofOfImplicationOrEquivalence() {}

std::vector<std::string> ProofOfImplicationOrEquivalence::query() {
    // clang-format off
    std::vector<std::string> tokens = {
        // "LINK_TEMPLATE", "Expression", "3",
        //     "NODE", "Symbol", "EVALUATION",
        //     "LINK_TEMPLATE", "Expression", "2",
        //         "NODE", "Symbol", "PREDICATE",
        //         "VARIABLE", "P",
        //     "VARIABLE", "C"
        "LINK_TEMPLATE", "Expression", "3",
        "NODE", "Symbol", "EVALUATION",
        "LINK_TEMPLATE", "Expression", "2",
            "NODE", "Symbol", "PREDICATE",
            "VARIABLE", "P",
        "LINK_TEMPLATE", "Expression", "2",
            "NODE", "Symbol", "CONCEPT",
            "VARIABLE", "C"
    };
    // clang-format on
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

std::vector<std::vector<std::string>> ProofOfImplicationOrEquivalence::get_requests() {
    std::vector<std::vector<std::string>> requests;
    // query + link creation template
    std::vector<std::string> query_and_link_creation_template(this->query());
    for (auto token : this->patterns_link_template()) {
        query_and_link_creation_template.push_back(token);
    }
    requests.push_back(query_and_link_creation_template);
    //  Not supported yet
    //  proof of implication
    ProofOfImplication proof_of_implication(first_handle, second_handle, max_proof_length, context);
    for (auto request : proof_of_implication.get_requests()) {
        requests.push_back(request);
    }
    // proof of equivalence
    ProofOfEquivalence proof_of_equivalence(first_handle, second_handle, max_proof_length, context);
    for (auto request : proof_of_equivalence.get_requests()) {
        requests.push_back(request);
    }

    return requests;
}

ProofOfImplication::ProofOfImplication(std::string first_handle,
                                       std::string second_handle,
                                       int max_proof_length,
                                       std::string context)
    : InferenceRequest(first_handle, second_handle, max_proof_length, context) {}

ProofOfImplication::~ProofOfImplication() {}

std::vector<std::string> ProofOfImplication::query() {
    // clang-format off
    std::vector<std::string> tokens = {
        "AND", "2",
            "LINK_TEMPLATE", "Expression", "2",
                "NODE", "Symbol", "SATISFYING_SET",
                "VARIABLE", "P1",
            "LINK_TEMPLATE", "Expression", "2",
                "NODE", "Symbol", "SATISFYING_SET",
                "VARIABLE", "P2",
            // "NOT",
            //     "OR",
            //         "LINK_TEMPLATE", "Expression", "3",
            //             "NODE", "Symbol", "IMPLICATION",
            //             "LINK_TEMPLATE", "Expression", "3",
            //                 "NODE", "Symbol", "EVALUATION",
            //                 "VARIABLE", "P1",
            //                 "VARIABLE", "C",
            //             "LINK_TEMPLATE", "Expression", "3",
            //                 "NODE", "Symbol", "EVALUATION",
            //                 "VARIABLE", "P2",
            //                 "VARIABLE", "C",
            //         "LINK_TEMPLATE", "Expression", "3",
            //             "NODE", "Symbol", "IMPLICATION",
            //             "LINK_TEMPLATE", "Expression", "3",
            //                 "NODE", "Symbol", "EVALUATION",
            //                 "VARIABLE", "P2",
            //                 "VARIABLE", "C",
            //             "LINK_TEMPLATE", "Expression", "3",
            //                 "NODE", "Symbol", "EVALUATION",
            //                 "VARIABLE", "P1",
            //                 "VARIABLE", "C"
    };
    // clang-format on
    return tokens;
}

std::string ProofOfImplication::get_type() { return "PROOF_OF_IMPLICATION"; }

std::vector<std::vector<std::string>> ProofOfImplication::get_requests() {
    std::vector<std::vector<std::string>> requests;
    auto query = this->query();
    query.push_back(this->get_type());  // processor
    requests.push_back(query);
    return requests;
}

ProofOfEquivalence::ProofOfEquivalence(std::string first_handle,
                                       std::string second_handle,
                                       int max_proof_length,
                                       std::string context)
    : InferenceRequest(first_handle, second_handle, max_proof_length, context) {}

ProofOfEquivalence::~ProofOfEquivalence() {}

std::vector<std::string> ProofOfEquivalence::query() {
    // clang-format off
    std::vector<std::string> tokens = {
        "AND", "2",
        "LINK_TEMPLATE", "Expression", "2",
            "NODE", "Symbol", "PATTERNS",
            "VARIABLE", "C1",
        "LINK_TEMPLATE", "Expression", "2",
            "NODE", "Symbol", "PATTERNS",
            "VARIABLE", "C2",
        // "NOT",
        //     "OR",
        //         "LINK_TEMPLATE", "Expression", "3",
        //             "NODE", "Symbol", "EQUIVALENCE",
        //             "LINK_TEMPLATE", "Expression", "3",
        //                 "NODE", "Symbol", "EVALUATION",
        //                 "VARIABLE", "P",
        //                 "VARIABLE", "C1",
        //             "LINK_TEMPLATE", "Expression", "3",
        //                 "NODE", "Symbol", "EVALUATION",
        //                 "VARIABLE", "P",
        //                 "VARIABLE", "C2",        
        //         "LINK_TEMPLATE", "Expression", "3",
        //             "NODE", "Symbol", "EQUIVALENCE",
        //             "LINK_TEMPLATE", "Expression", "3",
        //                 "NODE", "Symbol", "EVALUATION",
        //                 "VARIABLE", "P",
        //                 "VARIABLE", "C2",
        //             "LINK_TEMPLATE", "Expression", "3",
        //                 "NODE", "Symbol", "EVALUATION",
        //                 "VARIABLE", "P",
        //                 "VARIABLE", "C1"
    };
    // clang-format on
    return tokens;
}

std::string ProofOfEquivalence::get_type() { return "PROOF_OF_EQUIVALENCE"; }

std::vector<std::vector<std::string>> ProofOfEquivalence::get_requests() {
    std::vector<std::vector<std::string>> requests;
    auto query = this->query();
    query.push_back(this->get_type());  // processor
    requests.push_back(query);
    return requests;
}
