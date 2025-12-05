#include "InferenceRequest.h"

#include <memory>

#include "Atom.h"
#include "Hasher.h"
#include "Link.h"
#include "LinkProcessor.h"
#include "Node.h"
#include "UntypedVariable.h"
#include "Utils.h"
// #include "AtomDBSingleton.h"

using namespace std;
using namespace inference_agent;
using namespace link_creation_agent;
using namespace query_engine;
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

void InferenceRequest::set_repeat(unsigned int repeat) { this->repeat = repeat; }

unsigned int InferenceRequest::get_repeat() { return repeat; }

unsigned long InferenceRequest::get_lca_max_results() { return lca_max_results; }

void InferenceRequest::set_lca_max_results(unsigned long lca_max_results) {
    this->lca_max_results = lca_max_results;
}

unsigned int InferenceRequest::get_lca_max_repeats() { return lca_max_repeats; }

void InferenceRequest::set_lca_max_repeats(unsigned int lca_max_repeats) {
    this->lca_max_repeats = lca_max_repeats;
}

bool InferenceRequest::get_lca_update_attention_broker() { return lca_update_attention_broker; }

void InferenceRequest::set_lca_update_attention_broker(bool lca_update_attention_broker) {
    this->lca_update_attention_broker = lca_update_attention_broker;
}

string InferenceRequest::get_id() { return inference_request_id; }

void InferenceRequest::set_id(string inference_request_id) {
    this->inference_request_id = inference_request_id;
}

bool InferenceRequest::get_sent_evolution_request() { return sent_evolution_request; }

void InferenceRequest::set_sent_evolution_request(bool sent_evolution_request) {
    this->sent_evolution_request = sent_evolution_request;
}

string InferenceRequest::get_type() { return "INFERENCE_REQUEST"; }

string InferenceRequest::get_max_proof_length() { return to_string(max_proof_length); }

void InferenceRequest::set_timeout(unsigned int timeout) { this->timeout = timeout; }
unsigned int InferenceRequest::get_timeout() { return timeout; }

void InferenceRequest::set_full_evaluation(bool full_evaluation) {
    this->is_full_evaluation = full_evaluation;
}

bool InferenceRequest::get_full_evaluation() { return this->is_full_evaluation; }

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

static vector<string> inference_evolution_request_builder(
    string first_handle, string second_handle, string command, int max_proof_length, int& counter) {
    // clang-format off
    vector<string> query_template = {
        "LINK_TEMPLATE", "Expression",  "3",
        "NODE", "Symbol", "_TYPE_", 
        "_FIRST_",
        "_SECOND_", };
    // clang-format on
    vector<string> commands = {command};

    vector<string> request{};
    if (max_proof_length == 0) {
        return request;
    }
    vector<string> vars;
    vars.push_back(first_handle);
    for (int i = 0; i < max_proof_length; i++) {
        vars.push_back("V" + to_string(i));
    }
    vars.push_back(second_handle);
    auto commands_product = product<string>({commands}, vars.size() - 1);
    for (auto cp : commands_product) {
        counter++;
        request.push_back("AND");
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
             first_handle, second_handle, command, max_proof_length - 1, counter)) {
        request.push_back(tkn);
    }
    // }
    return request;
}

vector<string> InferenceRequest::get_distributed_inference_control_request() {
    vector<string> tokens;
    int size = 0;
    vector<string> request = inference_evolution_request_builder(
        first_handle, second_handle, this->command, max_proof_length, size);
    // tokens.push_back(this->context);
    if (size > 1) {
        tokens.push_back("OR");
        tokens.push_back(to_string(size));
    }
    for (auto token : request) {
        tokens.push_back(token);
    }
    return tokens;
}

vector<string> InferenceRequest::get_correlation_query() { return this->correlation_query; }
// clang-format off
// string correlation_query_str = 
// "OR 3 "
//     "AND 4 "
//         "LINK_TEMPLATE Expression 3 "
//             "NODE Symbol Evaluation "
//             "VARIABLE V0 "
//             "LINK_TEMPLATE Expression 2 "
//                 "NODE Symbol Concept "
//                 "VARIABLE S1 "
//         "LINK_TEMPLATE Expression 3 "
//             "NODE Symbol Contains "
//             "VARIABLE S1 "
//             "VARIABLE W1 "
//         "LINK_TEMPLATE Expression 3 "
//             "NODE Symbol Contains "
//             "VARIABLE S2 "
//             "VARIABLE W1 "
//         "LINK_TEMPLATE Expression 3 "
//             "NODE Symbol Evaluation "
//             "VARIABLE V1 "
//             "LINK_TEMPLATE Expression 2 "
//                 "NODE Symbol Concept "
//                 "VARIABLE S2 "
//     "LINK_TEMPLATE Expression 3 "
//         "NODE Symbol Implication "
//         "VARIABLE V0 "
//         "VARIABLE V1 "
//     "LINK_TEMPLATE Expression 3 "
//         "NODE Symbol Implication "
//         "VARIABLE V1 "
//         "VARIABLE V0";

string correlation_query_str = 
"OR 3 "
    "AND 4 "
        "LINK_TEMPLATE Expression 3 "
            "NODE Symbol Evaluation "
            "VARIABLE V0 "
            "LINK_TEMPLATE Expression 2 "
                "NODE Symbol Concept "
                "VARIABLE S1 "
        "LINK_TEMPLATE Expression 3 "
            "NODE Symbol Contains "
            "VARIABLE S1 "
            "VARIABLE W1 "
        "LINK_TEMPLATE Expression 3 "
            "NODE Symbol Contains "
            "VARIABLE S2 "
            "VARIABLE W1 "
        "LINK_TEMPLATE Expression 3 "
            "NODE Symbol Evaluation "
            "VARIABLE V1 "
            "LINK_TEMPLATE Expression 2 "
                "NODE Symbol Concept "
                "VARIABLE S2 "
    "LINK_TEMPLATE Expression 3 "
        "NODE Symbol Implication "
        "VARIABLE V0 "
        "VARIABLE V1 "
    "LINK_TEMPLATE Expression 3 "
        "NODE Symbol Implication "
        "VARIABLE V1 "
        "VARIABLE V0";

string correlation_constants_str = "V0";
string correlation_mapping_str = "V0:V1,V0:S2";

// clang-format on

void InferenceRequest::set_correlation_query(const string& query) {
    this->correlation_query = Utils::split(correlation_query_str, ' ');
}
map<string, QueryAnswerElement> InferenceRequest::get_correlation_query_constants() {
    return this->correlation_query_constants;
}
void InferenceRequest::set_correlation_query_constants(const string& constants) {
    auto constants_list = Utils::split(correlation_constants_str, ',');
    for (auto constant : constants_list) {
        this->correlation_query_constants[constant] = QueryAnswerElement(constant);
    }
}
map<string, QueryAnswerElement> InferenceRequest::get_correlation_mapping() {
    return this->correlation_mapping;
}
void InferenceRequest::set_correlation_mapping(const string& mapping) {
    auto mapping_list = Utils::split(correlation_mapping_str, ',');
    for (auto value : mapping_list) {
        auto pair = Utils::split(value, ':');
        if (pair.size() == 2) {
            this->correlation_mapping[pair[0]] = QueryAnswerElement(pair[1]);
        }
    }
}

vector<vector<string>> InferenceRequest::get_requests() { return {}; }

string InferenceRequest::get_context() { return context; }

vector<string> InferenceRequest::get_update_attention_allocation_query() { return {}; }

////////////////////////////// IMPLICATION ///////////////////////////////////

ProofOfImplication::ProofOfImplication(string first_handle,
                                       string second_handle,
                                       int max_proof_length,
                                       string context)
    : InferenceRequest(first_handle, second_handle, max_proof_length, context) {
    this->command = "Implication";
}

ProofOfImplication::ProofOfImplication() : InferenceRequest() { this->command = "Implication"; }

vector<string> ProofOfImplication::query() {
    // clang-format off
    vector<string> tokens = {
        // "AND", "2",
        //     "LINK_TEMPLATE", "Expression", "2",
        //         "NODE", "Symbol", "Predicate",
        //         "VARIABLE", "P1",
        //     "LINK_TEMPLATE", "Expression", "2",
        //         "NODE", "Symbol", "Predicate",
        //         "VARIABLE", "P2",

        "AND", "2",
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "Evaluation",
                "VARIABLE", "P1",
                "VARIABLE", "C",
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "Evaluation",
                "VARIABLE", "P2",
                "VARIABLE", "C",


        // "AND", "2",
        //     "LINK_TEMPLATE", "Expression", "3",
        //         "NODE", "Symbol", "Evaluation",
        //         "VARIABLE", "P1",
        //         "VARIABLE", "C1",
        //     "LINK_TEMPLATE", "Expression", "3",
        //         "NODE", "Symbol", "Evaluation",
        //         "VARIABLE", "P2",
        //         "VARIABLE", "C2",
            // "NOT",
            //     "OR",
            //         "LINK_TEMPLATE", "Expression", "3",
            //             "NODE", "Symbol", "Implication",
            //             "LINK_TEMPLATE", "Expression", "3",
            //                 "NODE", "Symbol", "Evaluation",
            //                 "VARIABLE", "P1",
            //                 "VARIABLE", "C",
            //             "LINK_TEMPLATE", "Expression", "3",
            //                 "NODE", "Symbol", "Evaluation",
            //                 "VARIABLE", "P2",
            //                 "VARIABLE", "C",
            //         "LINK_TEMPLATE", "Expression", "3",
            //             "NODE", "Symbol", "Implication",
            //             "LINK_TEMPLATE", "Expression", "3",
            //                 "NODE", "Symbol", "Evaluation",
            //                 "VARIABLE", "P2",
            //                 "VARIABLE", "C",
            //             "LINK_TEMPLATE", "Expression", "3",
            //                 "NODE", "Symbol", "Evaluation",
            //                 "VARIABLE", "P1",
            //                 "VARIABLE", "C"
    };
    // clang-format on
    return tokens;
}

string ProofOfImplication::get_type() {
    return LinkCreationProcessor::get_processor_type(ProcessorType::IMPLICATION_RELATION);
}

vector<vector<string>> ProofOfImplication::get_requests() {
    vector<vector<string>> requests;
    ProofOfEquivalence proof_of_equivalence;
    auto query = this->query();
    query.push_back(this->get_type());  // processor
    requests.push_back(query);
    auto equiv_query = proof_of_equivalence.query();
    equiv_query.push_back(proof_of_equivalence.get_type());  // processor
    requests.push_back(equiv_query);
    return requests;
}

string ProofOfImplication::get_direct_inference_hash() {
    Link implication("Expression",
                     {Hasher::node_handle("Symbol", "Implication"), first_handle, second_handle});
    return implication.handle();
}

vector<string> ProofOfImplication::get_update_attention_allocation_query() {
    // clang-format off
    vector<string> tokens = {
    // "OR", "2",
    //    "LINK_TEMPLATE", "Expression", "3",
    //         "NODE", "Symbol", "Evaluation",
    //         "ATOM", this->first_handle,
    //         "LINK_TEMPLATE", "Expression", "2",
    //             "NODE", "Symbol", "Concept",
    //             "VARIABLE", "TARGET",
    //     "LINK_TEMPLATE", "Expression", "3",
    //         "NODE", "Symbol", "Evaluation",
    //         "ATOM", this->second_handle,
    //         "LINK_TEMPLATE", "Expression", "2",
    //             "NODE", "Symbol", "Concept",
    //             "VARIABLE", "TARGET",

    "OR", "2",
       "LINK_TEMPLATE", "Expression", "3",
            "NODE", "Symbol", "Evaluation",
            "ATOM", this->first_handle,
            "VARIABLE", "TARGET",
        "LINK_TEMPLATE", "Expression", "3",
            "NODE", "Symbol", "Evaluation",
            "ATOM", this->second_handle,
            "VARIABLE", "TARGET",


    // "OR", "2",
    //    "LINK_TEMPLATE", "Expression", "2",
    //         "VARIABLE", "Predicate",
    //         "ATOM", this->first_handle,
    //     "LINK_TEMPLATE", "Expression", "2",
    //         "VARIABLE", "PREDICATE2",
    //         "ATOM", this->second_handle,
    };
    // clang-format on
    return tokens;
}

////////////////////////////// EQUIVALENCE ///////////////////////////////////

ProofOfEquivalence::ProofOfEquivalence(string first_handle,
                                       string second_handle,
                                       int max_proof_length,
                                       string context)
    : InferenceRequest(first_handle, second_handle, max_proof_length, context) {
    this->command = "Equivalence";
}
ProofOfEquivalence::ProofOfEquivalence() : InferenceRequest() { this->command = "Equivalence"; }

vector<string> ProofOfEquivalence::query() {
    // clang-format off
    vector<string> tokens = {
        // "AND", "2",
        // "LINK_TEMPLATE", "Expression", "2",
        //     "NODE", "Symbol", "Concept",
        //     "VARIABLE", "C1",
        // "LINK_TEMPLATE", "Expression", "2",
        //     "NODE", "Symbol", "Concept",
        //     "VARIABLE", "C2",
        "AND", "2",
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "Evaluation",
                "VARIABLE", "P",
                "VARIABLE", "C1",
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "Evaluation",
                "VARIABLE", "P",
                "VARIABLE", "C2",
        // "NOT",
        //     "OR",
        //         "LINK_TEMPLATE", "Expression", "3",
        //             "NODE", "Symbol", "Equivalence",
        //             "LINK_TEMPLATE", "Expression", "3",
        //                 "NODE", "Symbol", "Evaluation",
        //                 "VARIABLE", "P",
        //                 "VARIABLE", "C1",
        //             "LINK_TEMPLATE", "Expression", "3",
        //                 "NODE", "Symbol", "Evaluation",
        //                 "VARIABLE", "P",
        //                 "VARIABLE", "C2",        
        //         "LINK_TEMPLATE", "Expression", "3",
        //             "NODE", "Symbol", "Equivalence",
        //             "LINK_TEMPLATE", "Expression", "3",
        //                 "NODE", "Symbol", "Evaluation",
        //                 "VARIABLE", "P",
        //                 "VARIABLE", "C2",
        //             "LINK_TEMPLATE", "Expression", "3",
        //                 "NODE", "Symbol", "Evaluation",
        //                 "VARIABLE", "P",
        //                 "VARIABLE", "C1"
    };
    // clang-format on
    return tokens;
}

string ProofOfEquivalence::get_type() {
    return LinkCreationProcessor::get_processor_type(ProcessorType::EQUIVALENCE_RELATION);
}

vector<vector<string>> ProofOfEquivalence::get_requests() {
    vector<vector<string>> requests;
    ProofOfImplication proof_of_implication;
    auto query = this->query();
    query.push_back(this->get_type());  // processor
    requests.push_back(query);
    auto impl_query = proof_of_implication.query();
    impl_query.push_back(proof_of_implication.get_type());  // processor
    requests.push_back(impl_query);
    return requests;
}

string ProofOfEquivalence::get_direct_inference_hash() {
    Link equivalence("Expression",
                     {Hasher::node_handle("Symbol", "Equivalence"), first_handle, second_handle});
    return equivalence.handle();
}

vector<string> ProofOfEquivalence::get_update_attention_allocation_query() {
    // clang-format off
    vector<string> tokens = {
        "OR", "2",
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "Evaluation",
                "VARIABLE", "TARGET",
                "ATOM", this->first_handle,
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "Evaluation",
                "VARIABLE", "TARGET",
                "ATOM", this->second_handle,

    };
    // clang-format on
    return tokens;
}