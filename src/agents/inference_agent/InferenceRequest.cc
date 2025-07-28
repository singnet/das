#include "InferenceRequest.h"

#include <memory>

#include "Link.h"
#include "Atom.h"
#include "LinkCreateTemplate.h"
#include "Node.h"
#include "UntypedVariable.h"
#include "Utils.h"
#include "AtomDBSingleton.h"

using namespace std;
using namespace inference_agent;
using namespace link_creation_agent;
using namespace commons;
using namespace atoms;


InferenceRequest::InferenceRequest(string first_handle,
                                   string second_handle,
                                   int max_proof_length,
                                   string context,
                                   string max_answers,
                                   string update_attention_broker) {
    this->first_handle = first_handle;
    this->second_handle = second_handle;
    this->max_proof_length = max_proof_length;
    this->context = context;
}

InferenceRequest::~InferenceRequest() {}

vector<string> InferenceRequest::query() { return {}; }

string InferenceRequest::get_id() { return inference_request_id; }

void InferenceRequest::set_id(string inference_request_id) {
    this->inference_request_id = inference_request_id;
}

string InferenceRequest::get_type() { return "INFERENCE_REQUEST"; }

string InferenceRequest::get_max_proof_length() { return to_string(max_proof_length); }

void InferenceRequest::set_timeout(unsigned int timeout) { this->timeout = timeout; }
unsigned int InferenceRequest::get_timeout() { return timeout; }

template <typename T>
static vector<vector<T>> product(const vector<T>& iterable, size_t repeat) {
    vector<vector<T>> result;
    vector<T> current(repeat, iterable[0]);
    result.push_back(current);

    for (size_t i = 0; i < repeat; i++) {
        vector<vector<T>> temp;
        for (const auto& item : iterable) {
            for (const auto& vec : result) {
                vector<T> new_vec = vec;
                new_vec[i] = item;
                temp.push_back(new_vec);
            }
        }
        result = move(temp);
    }

    return result;
}

static vector<string> inference_evolution_request_builder(string first_handle,
                                                          string second_handle,
                                                          int max_proof_length,
                                                          int& counter) {
    // clang-format off
    vector<string> query_template = {
        "LINK_TEMPLATE", "Expression",  "3",
        "NODE", "Symbol", "_TYPE_", 
        "_FIRST_",
        "_SECOND_", };
    // clang-format on
    vector<string> commands = {"IMPLICATION", "EQUIVALENCE"};
    vector<string> request;
    if (max_proof_length == 1) {
        counter += commands.size();
        for (auto ttype : commands) {
            for (auto token : query_template) {
                if (token == "_TYPE_") {
                    request.push_back(ttype);
                } else if (token == "_FIRST_") {
                    request.push_back("ATOM");
                    request.push_back(first_handle);
                } else if (token == "_SECOND_") {
                    request.push_back("ATOM");
                    request.push_back(second_handle);
                } else if (token == "LINK_TEMPLATE"){
                    request.push_back("LINK");
                } else {
                    request.push_back(token);
                }
            }
        }
    } else {
        vector<string> vars;
        vars.push_back(first_handle);
        for (int i = 1; i < max_proof_length; i++) {
            vars.push_back("V" + to_string(i));
        }
        vars.push_back(second_handle);
        auto commands_product = product<string>({commands}, vars.size() - 1);
        for (auto cp : commands_product) {
            counter++;
            request.push_back("OR");
            request.push_back(to_string(vars.size() - 1));
            for (long unsigned int i = 1; i < vars.size(); i++) {
                for (auto token : query_template) {
                    if (token == "_TYPE_") {
                        request.push_back(cp[i - 1]);
                    } else if (token == "_FIRST_") {
                        request.push_back((vars[i - 1] == first_handle || vars[i - 1] == second_handle)
                                              ? "ATOM"
                                              : "VARIABLE");
                        request.push_back(vars[i - 1]);
                    } else if (token == "_SECOND_") {
                        request.push_back(
                            (vars[i] == first_handle || vars[i] == second_handle) ? "ATOM" : "VARIABLE");
                        request.push_back(vars[i]);
                    } else {
                        request.push_back(token);
                    }
                }
            }
        }

        for (auto tkn : inference_evolution_request_builder(
                 first_handle, second_handle, max_proof_length - 1, counter)) {
            request.push_back(tkn);
        }
    }
    return request;
}

static vector<string> tokenize_atom(const string& handle) {
    vector<string> tokens;
    shared_ptr<Atom> atom = atomdb::AtomDBSingleton::get_instance()->get_atom(handle);
    if (Atom::is_node(*atom)) {
        shared_ptr<Node> node = dynamic_pointer_cast<Node>(atom);
        tokens.push_back("NODE");
        tokens.push_back(node->type);
        tokens.push_back(node->name);
    } else if (Atom::is_link(*atom)) {
        shared_ptr<Link> link = dynamic_pointer_cast<Link>(atom);
        tokens.push_back("LINK");
        tokens.push_back(link->type);
        tokens.push_back(to_string(link->targets.size()));
        for (const auto& target : link->targets) {
            for (auto token : tokenize_atom(target)) {
                tokens.push_back(token);
            }
        }
    } else {
        Utils::error("Unknown atom type: " + atom->type);
    }
    return tokens;
}


vector<string> InferenceRequest::get_distributed_inference_control_request() {
    vector<string> tokens;
    int size = 0;

    vector<string> request =
        inference_evolution_request_builder(first_handle, second_handle, max_proof_length, size);
    // tokens.push_back(this->context);
    tokens.push_back("OR");
    tokens.push_back(to_string(size));
    for (auto token : request) {
        tokens.push_back(token);
    }
    // for (size_t i = 0; i < request.size(); i++) {
    //     if (request[i] == "ATOM") {
    //         for(auto token : tokenize_atom(request[i + 1])) {
    //             tokens.push_back(token);
    //         }
    //         i++;
    //     } else {
    //         tokens.push_back(request[i]);
    //     }
    // }
    return tokens;
}

vector<vector<string>> InferenceRequest::get_requests() { return {}; }

string InferenceRequest::get_context() { return context; }

ProofOfImplicationOrEquivalence::ProofOfImplicationOrEquivalence(string first_handle,
                                                                 string second_handle,
                                                                 int max_proof_length,
                                                                 string context)
    : InferenceRequest(first_handle, second_handle, max_proof_length, context) {}

ProofOfImplicationOrEquivalence::~ProofOfImplicationOrEquivalence() {}

vector<string> ProofOfImplicationOrEquivalence::query() {
    // clang-format off
    vector<string> tokens = {
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

vector<string> ProofOfImplicationOrEquivalence::patterns_link_template() {
    // SATISFYING_SET
    LinkCreateTemplate satisfying_set_link_template = LinkCreateTemplate("Expression");

    shared_ptr<Node> evaluation_node = make_shared<Node>("Symbol", "SATISFYING_SET");
    shared_ptr<UntypedVariable> evaluation_variable = make_shared<UntypedVariable>("P");
    satisfying_set_link_template.add_target(evaluation_node);
    satisfying_set_link_template.add_target(evaluation_variable);
    auto custom_field = CustomField("truth_value");
    custom_field.add_field("strength", "1.0");
    custom_field.add_field("confidence", "1.0");
    satisfying_set_link_template.add_custom_field(custom_field);

    // PATTERNS
    LinkCreateTemplate patterns_link_template = LinkCreateTemplate("Expression");

    shared_ptr<Node> patterns_node = make_shared<Node>("Symbol", "PATTERNS");
    shared_ptr<UntypedVariable> patterns_variable = make_shared<UntypedVariable>("C");
    patterns_link_template.add_target(patterns_node);
    patterns_link_template.add_target(patterns_variable);
    auto patterns_custom_field = CustomField("truth_value");
    patterns_custom_field.add_field("strength", "1.0");
    patterns_custom_field.add_field("confidence", "1.0");
    patterns_link_template.add_custom_field(patterns_custom_field);

    vector<string> tokens;
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

string ProofOfImplicationOrEquivalence::get_type() { return "PROOF_OF_IMPLICATION_OR_EQUIVALENCE"; }

vector<vector<string>> ProofOfImplicationOrEquivalence::get_requests() {
    vector<vector<string>> requests;
    // query + link creation template
    vector<string> query_and_link_creation_template(this->query());
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

ProofOfImplication::ProofOfImplication(string first_handle,
                                       string second_handle,
                                       int max_proof_length,
                                       string context)
    : InferenceRequest(first_handle, second_handle, max_proof_length, context) {}

ProofOfImplication::~ProofOfImplication() {}

vector<string> ProofOfImplication::query() {
    // clang-format off
    vector<string> tokens = {
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

string ProofOfImplication::get_type() { return "PROOF_OF_IMPLICATION"; }

vector<vector<string>> ProofOfImplication::get_requests() {
    vector<vector<string>> requests;
    auto query = this->query();
    query.push_back(this->get_type());  // processor
    requests.push_back(query);
    return requests;
}

ProofOfEquivalence::ProofOfEquivalence(string first_handle,
                                       string second_handle,
                                       int max_proof_length,
                                       string context)
    : InferenceRequest(first_handle, second_handle, max_proof_length, context) {}

ProofOfEquivalence::~ProofOfEquivalence() {}

vector<string> ProofOfEquivalence::query() {
    // clang-format off
    vector<string> tokens = {
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

string ProofOfEquivalence::get_type() { return "PROOF_OF_EQUIVALENCE"; }

vector<vector<string>> ProofOfEquivalence::get_requests() {
    vector<vector<string>> requests;
    auto query = this->query();
    query.push_back(this->get_type());  // processor
    requests.push_back(query);
    return requests;
}
