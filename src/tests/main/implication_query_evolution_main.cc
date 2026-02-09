#include <signal.h>

#define LOG_LEVEL LOCAL_DEBUG_LEVEL

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "AttentionBrokerClient.h"
#include "ContextBrokerProxy.h"
#include "FitnessFunctionRegistry.h"
#include "Hasher.h"
#include "Logger.h"
#include "PatternMatchingQueryProxy.h"
#include "QueryAnswer.h"
#include "QueryEvolutionProxy.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"

#define MAX_QUERY_ANSWERS ((unsigned int) 100000)

// Symbols
#define AND_OPERATOR "AND"
#define OR_OPERATOR "OR"
#define LINK_TEMPLATE "LINK_TEMPLATE"
#define LINK "LINK"
#define NODE "NODE"
#define VARIABLE "VARIABLE"
#define ATOM "ATOM"
#define EXPRESSION "Expression"
#define SYMBOL "Symbol"
#define SENTENCE "Sentence"
#define WORD "Word"
#define CONTAINS "Contains"
#define EVALUATION "Evaluation"
#define PREDICATE "Predicate"
#define CONCEPT "Concept"
#define EQUIVALENCE "Equivalence"
#define IMPLICATION "Implication"

// Variables
#define V1 "v1"
#define V2 "v2"
#define V3 "v3"
#define PREDICATE1 "predicate1"
#define PREDICATE2 "predicate2"
#define PREDICATE3 "predicate3"
#define CONCEPT1 "concept1"
#define CONCEPT2 "concept2"
#define SENTENCE1 "sentence1"
#define SENTENCE2 "sentence2"
#define WORD1 "word1"
#define P1 "P1"
#define P2 "P2"
#define C1 "C1"
#define C2 "C2"
#define C "C"
#define P "P"

// Misc
#define STRENGTH "strength"
#define IS_LITERAL "is_literal"
#define FITNESS_FUNCTION "multiply_strength"

static string IMPLICATION_HANDLE = Hasher::node_handle(SYMBOL, IMPLICATION);
static string EQUIVALENCE_HANDLE = Hasher::node_handle(SYMBOL, EQUIVALENCE);
static string PREDICATE_HANDLE = Hasher::node_handle(SYMBOL, PREDICATE);
static string EVALUATION_HANDLE = Hasher::node_handle(SYMBOL, EVALUATION);
static float RENT_RATE = 0.25;
static float SPREADING_RATE_LOWERBOUND = 0.90;
static float SPREADING_RATE_UPPERBOUND = 0.90;
static double SELECTION_RATE = 0.10;
static double ELITISM_RATE = 0.08;
static unsigned int LINK_BUILDING_QUERY_SIZE = 150;
static unsigned int POPULATION_SIZE = 50;
static unsigned int MAX_GENERATIONS = 20;
static unsigned int NUM_ITERATIONS = 1;

static bool CONTEXT_CREATION_ONLY = false;
static bool SAVE_NEW_LINKS = true;
static string NEW_LINKS_FILE_NAME = "newly_created_links.txt";
static string CONTEXT_FILE_NAME = "_CONTEXT_DUMP";

using namespace std;
using namespace atomdb;
using namespace query_engine;
using namespace evolution;
using namespace service_bus;
using namespace attention_broker;
using namespace context_broker;

static shared_ptr<AtomDB> db;
static shared_ptr<ServiceBus> bus;
static vector<vector<string>> buffer_determiners;
static map<string, vector<string>> weight_calculation_cache;

static void save_link(Link& link) {
    ofstream file;
    file.open(NEW_LINKS_FILE_NAME, std::ios::app);
    if (file.is_open()) {
        vector<string> tokens;
        link.tokenize(tokens);
        for (unsigned int i = 0; i < tokens.size(); i++) {
            file << tokens[i];
            if (i != tokens.size() - 1) {
                file << " ";
            }
        }
        file << endl;
        file.close();
    } else {
        Utils::error("Couldn't open file for writing: " + NEW_LINKS_FILE_NAME);
    }
}

static string get_predicate_name(string handle) {
    shared_ptr<Link> predicate_link = dynamic_pointer_cast<Link>(db->get_atom(handle));
    shared_ptr<Node> predicate_symbol =
        dynamic_pointer_cast<Node>(db->get_atom(predicate_link->targets[1]));
    return predicate_symbol->name;
}

/*
static double get_strength(string handle) {
    shared_ptr<Atom> atom = db->get_atom(handle);
    double strength = 0;
    if (atom != nullptr) {
        strength = atom->custom_attributes.get_or<double>(STRENGTH, 0);
    }
    return strength;
}
*/

static void print_answer(shared_ptr<QueryAnswer> query_answer) {
    unsigned int chain_size = query_answer->handles.size();
    cout << fixed << setw(6) << setprecision(4) << setfill('0');
    cout << query_answer->strength << " [" << to_string(chain_size) << "]: ";
    bool first_flag = true;
    string source_predicate, target_predicate, last_predicate;
    for (string implication_handle : query_answer->handles) {
        shared_ptr<Link> implication_link = dynamic_pointer_cast<Link>(db->get_atom(implication_handle));
        if (first_flag) {
            first_flag = false;
            last_predicate = source_predicate = get_predicate_name(implication_link->targets[1]);
            cout << source_predicate;
        }
        // if (source_predicate != last_predicate) {
        //     Utils::error("Bad predicate chaining: " + source_predicate + " != " + last_predicate);
        // }
        target_predicate = get_predicate_name(implication_link->targets[2]);
        cout << " --> " << target_predicate;
        last_predicate = target_predicate;
    }
    cout << endl;
}

static shared_ptr<PatternMatchingQueryProxy> issue_link_building_query(
    const vector<string>& query_tokens, const string& context, unsigned int max_answers) {
    auto proxy = make_shared<PatternMatchingQueryProxy>(query_tokens, context);
    proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = true;
    proxy->parameters[BaseQueryProxy::ATTENTION_UPDATE_FLAG] = false;
    // proxy->parameters[BaseQueryProxy::USE_LINK_TEMPLATE_CACHE] = true; // Use the default value
    proxy->parameters[PatternMatchingQueryProxy::MAX_ANSWERS] = (unsigned int) max_answers;
    proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = true;
    proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] = false;
    proxy->parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] = false;
    proxy->parameters[PatternMatchingQueryProxy::UNIQUE_VALUE_FLAG] = true;

    ServiceBusSingleton::get_instance()->issue_bus_command(proxy);
    return proxy;
}

// Fazer um cache que permita lembrar dos handles da resposta do QA e recalcular apenas os counts (que
// dependem dos weight)
static shared_ptr<PatternMatchingQueryProxy> issue_weight_count_query(const vector<string>& query_tokens,
                                                                      const string& context) {
    auto proxy = make_shared<PatternMatchingQueryProxy>(query_tokens, context);
    proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = true;
    proxy->parameters[BaseQueryProxy::ATTENTION_UPDATE_FLAG] = false;
    proxy->parameters[BaseQueryProxy::USE_LINK_TEMPLATE_CACHE] = true;
    proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = false;
    proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] = false;
    proxy->parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] = false;

    ServiceBusSingleton::get_instance()->issue_bus_command(proxy);
    return proxy;
}

static shared_ptr<PatternMatchingQueryProxy> issue_attention_allocation_query(
    const vector<string>& query_tokens, const string& context) {
    auto proxy = make_shared<PatternMatchingQueryProxy>(query_tokens, context);
    proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = true;
    proxy->parameters[BaseQueryProxy::ATTENTION_UPDATE_FLAG] = false;
    proxy->parameters[BaseQueryProxy::USE_LINK_TEMPLATE_CACHE] = false;
    proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = false;
    proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] = false;
    proxy->parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] = false;

    ServiceBusSingleton::get_instance()->issue_bus_command(proxy);
    return proxy;
}

static void update_attention_allocation(const string& predicate_source,
                                        const string& predicate_target,
                                        const string& context) {
    LOG_INFO("Updating attention allocation using custom tarversing.");

    // clang-format off
    vector<string> attention_update_query1 = {
        OR_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                ATOM, predicate_source,
                VARIABLE, SENTENCE1,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                ATOM, predicate_target,
                VARIABLE, SENTENCE1
    };

    vector<string> attention_update_query2 = {
        LINK_TEMPLATE, EXPRESSION, "3",
            NODE, SYMBOL, EVALUATION,
            VARIABLE, PREDICATE1,
            ATOM, "sentence_placeholder"
    };
    // clang-format on

    shared_ptr<PatternMatchingQueryProxy> proxy1 =
        issue_attention_allocation_query(attention_update_query1, context);
    shared_ptr<PatternMatchingQueryProxy> proxy2;
    set<string> to_correlate;
    map<string, unsigned int> to_stimulate;
    shared_ptr<QueryAnswer> query_answer1;
    shared_ptr<QueryAnswer> query_answer2;
    unsigned int count = 0;
    while (!proxy1->finished()) {
        if ((query_answer1 = proxy1->pop()) == NULL) {
            Utils::sleep();
        } else {
            LOG_INFO("Traversing handle " + to_string(++count) + "/" + to_string(proxy1->get_count()));
            attention_update_query2[attention_update_query2.size() - 1] = query_answer1->get(SENTENCE1);
            proxy2 = issue_attention_allocation_query(attention_update_query2, context);
            // to_stimulate[query_answer1->get(0)] = 1;
            to_stimulate[query_answer1->get(SENTENCE1)] = 1;
            while (!proxy2->finished()) {
                if ((query_answer2 = proxy2->pop()) == NULL) {
                    Utils::sleep();
                } else {
                    // to_stimulate[query_answer2->get(0)] = 1;
                    // to_stimulate[query_answer2->get(PREDICATE1)] = 1;
                    // to_correlate.insert(query_answer1->get(0));
                    to_correlate.insert(query_answer1->get(SENTENCE1));
                    // to_correlate.insert(query_answer2->get(0));
                    to_correlate.insert(query_answer2->get(PREDICATE1));
                    AttentionBrokerClient::correlate(to_correlate, context);
                    to_correlate.clear();
                }
            }
        }
    }
    AttentionBrokerClient::stimulate(to_stimulate, context);
    LOG_INFO("Updated attention for " + to_string(to_stimulate.size()) + " handles.");
}

/*
static void update_context(const string& predicate_source, const string& predicate_target,
shared_ptr<ContextBrokerProxy> context_proxy) {

    LOG_INFO("Updating context using custom tarversing.");

    vector<string> attention_update_query1 = {
        OR_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                ATOM, predicate_source,
                LINK_TEMPLATE, EXPRESSION, "2",
                    NODE, SYMBOL, CONCEPT,
                    VARIABLE, SENTENCE1,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                ATOM, predicate_target,
                LINK_TEMPLATE, EXPRESSION, "2",
                    NODE, SYMBOL, CONCEPT,
                    VARIABLE, SENTENCE1
    };

    vector<string> attention_update_query2 = {
        LINK_TEMPLATE, EXPRESSION, "3",
            NODE, SYMBOL, EVALUATION,
            VARIABLE, PREDICATE1,
            LINK, EXPRESSION, "2",
                NODE, SYMBOL, CONCEPT,
                ATOM, "sentence"
    };

    shared_ptr<PatternMatchingQueryProxy> proxy1 =
issue_attention_allocation_query(attention_update_query1, context); shared_ptr<PatternMatchingQueryProxy>
proxy2; set<string> to_correlate; map<string, unsigned int> to_stimulate; shared_ptr<QueryAnswer>
query_answer1; shared_ptr<QueryAnswer> query_answer2; unsigned int count = 0; while (!proxy1->finished())
{ if ((query_answer1 = proxy1->pop()) == NULL) { Utils::sleep(); } else { LOG_INFO("Traversing handle " +
to_string(++count) + "/" + to_string(proxy1->get_count()));
            attention_update_query2[attention_update_query2.size() - 1] = query_answer1->get(SENTENCE1);
            proxy2 = issue_attention_allocation_query(attention_update_query2, context);
            to_stimulate[query_answer1->get(0)] = 1;
            to_stimulate[query_answer1->get(SENTENCE1)] = 1;
            while (!proxy2->finished()) {
                if ((query_answer2 = proxy2->pop()) == NULL) {
                    Utils::sleep();
                } else {
                    to_stimulate[query_answer2->get(0)] = 1;
                    to_stimulate[query_answer2->get(PREDICATE1)] = 1;
                    to_correlate.insert(query_answer1->get(0));
                    to_correlate.insert(query_answer1->get(SENTENCE1));
                    to_correlate.insert(query_answer2->get(0));
                    to_correlate.insert(query_answer2->get(PREDICATE1));
                    context_proxy::correlate(to_correlate);
                    to_correlate.clear();
                }
            }
        }
    }
    context_proxy::stimulate(to_stimulate);
    LOG_INFO("Updated attention for " + to_string(to_stimulate.size()) + " handles.");
}
*/

static void insert_or_update(map<string, double>& count_map, const string& key, double value) {
    auto iterator = count_map.find(key);
    if (iterator == count_map.end()) {
        count_map[key] = value;
    } else {
        if (value > iterator->second) {
            count_map[key] = value;
        }
    }
}

static void compute_counts(const vector<vector<string>>& query_tokens,
                           const string& context,
                           const QueryAnswerElement& target_element,
                           const string& handle0,
                           const string& handle1,
                           double& count_0,
                           double& count_1,
                           double& count_intersection,
                           double& count_union) {
    LOG_LOCAL_DEBUG("Computing counts for " + handle0 + " and " + handle1);
    shared_ptr<PatternMatchingQueryProxy> proxy[2];

    count_0 = 0.0;
    count_1 = 0.0;
    count_intersection = 0.0;
    count_union = 0.0;

    map<string, double> count_map[2];
    map<string, double> count_map_union;
    map<string, double> count_map_intersection;

    double d;
    string handle;
    for (unsigned int i = 0; i < 2; i++) {
        proxy[i] = issue_weight_count_query(query_tokens[i], context);
    }
    LOG_LOCAL_DEBUG("Queries issued");
    shared_ptr<QueryAnswer> query_answer;
    for (unsigned int i = 0; i < 2; i++) {
        LOG_LOCAL_DEBUG("i: " + to_string(i));
        ServiceBusSingleton::get_instance()->issue_bus_command(proxy[i]);
        while (!proxy[i]->finished()) {
            if ((query_answer = proxy[i]->pop()) == NULL) {
                Utils::sleep();
            } else {
                if (query_answer->handles.size() == 1) {
                    d = 1.0;
                } else {
                    string link_handle = query_answer->handles[1];
                    auto link = db->get_atom(link_handle);
                    d = link->custom_attributes.get<double>(STRENGTH);
                }
                handle = query_answer->get(target_element);
                insert_or_update(count_map[i], handle, d);
            }
        }
    }
    LOG_LOCAL_DEBUG("Query answers processed");
    count_map_union = count_map[0];
    for (auto pair : count_map[1]) {
        auto iterator = count_map[0].find(pair.first);
        if (iterator == count_map[0].end()) {
            insert_or_update(count_map_union, pair.first, pair.second);
        } else {
            insert_or_update(count_map_union, pair.first, pair.second);
            insert_or_update(count_map_intersection, pair.first, min(pair.second, iterator->second));
        }
    }

    for (auto pair : count_map_intersection) {
        count_intersection += pair.second;
    }
    for (auto pair : count_map_union) {
        count_union += pair.second;
    }
    for (auto pair : count_map[0]) {
        count_0 += pair.second;
    }
    for (auto pair : count_map[1]) {
        count_1 += pair.second;
    }
    LOG_DEBUG("Counts: " + to_string(count_0) + " " + to_string(count_1) + " " +
              to_string(count_intersection) + " " + to_string(count_union));
}

static Link add_or_update_link(const string& type_handle,
                               const string& target1,
                               const string& target2,
                               double strength) {
    LOG_DEBUG("add_or_update_link(" + type_handle + ", " + target1 + ", " + target2 + ", " +
              to_string(strength) + ")");
    Link new_link(EXPRESSION, {type_handle, target1, target2}, true, {{STRENGTH, strength}});
    LOG_DEBUG("Add or update: " + new_link.to_string());
    string handle = new_link.handle();
    if (db->link_exists(handle)) {
        auto old_link = db->get_atom(handle);
        LOG_DEBUG("Link already exists: " + old_link->to_string());
        if (strength > old_link->custom_attributes.get<double>(STRENGTH)) {
            LOG_DEBUG("Updating");
            db->delete_link(handle, false);
            db->add_link(&new_link);
            if (SAVE_NEW_LINKS) {
                save_link(new_link);
            }
        }
    } else {
        db->add_link(&new_link);
        buffer_determiners.push_back({handle, target1, target2});
        if (SAVE_NEW_LINKS) {
            save_link(new_link);
        }
    }
    return new_link;
}

static Link add_predicate(const string& predicate_name) {
    LOG_DEBUG("add_predicate(" + predicate_name + ")");
    Node new_node(SYMBOL, predicate_name, {{IS_LITERAL, true}});
    Link new_link(EXPRESSION, {PREDICATE_HANDLE, new_node.handle()}, false);
    if (!db->node_exists(new_node.handle())) {
        db->add_node(&new_node);
        db->add_link(&new_link);
    }
    return new_link;
}

static void build_implication_link(shared_ptr<QueryAnswer> query_answer, const string& context) {
    if (query_answer->handles[0] == query_answer->handles[1]) {
        LOG_DEBUG("Skipping because handles are the same: " + query_answer->handles[0]);
        return;
    }

    string predicate[2] = {query_answer->assignment.get("P1"), query_answer->assignment.get("P2")};
    LOG_DEBUG("Evaluating implication: " + get_predicate_name(predicate[0]) + " ==> " +
              get_predicate_name(predicate[1]));
    vector<vector<string>> query;

    for (unsigned int i = 0; i < 2; i++) {
        // clang-format off
        query.push_back({
            OR_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                ATOM, predicate[i],
                VARIABLE, CONCEPT1,
            AND_OPERATOR, "2",
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, EVALUATION,
                    ATOM, predicate[i],
                    VARIABLE, CONCEPT2,
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, EQUIVALENCE,
                    VARIABLE, CONCEPT2,
                    VARIABLE, CONCEPT1,
        });
        // clang-format on
    }
    double count_0, count_1, count_intersection, count_union;
    QueryAnswerElement target_element(CONCEPT1);
    compute_counts(query,
                   context,
                   target_element,
                   predicate[0],
                   predicate[1],
                   count_0,
                   count_1,
                   count_intersection,
                   count_union);
    if (count_intersection > 0) {
        if (count_0 > 0) {
            double strength = count_intersection / count_0;
            add_or_update_link(IMPLICATION_HANDLE, predicate[0], predicate[1], strength);
        }
        if (count_1 > 0) {
            double strength = count_intersection / count_1;
            add_or_update_link(IMPLICATION_HANDLE, predicate[1], predicate[0], strength);
        }
    }
}

static void build_and_predicate_link(shared_ptr<QueryAnswer> query_answer, const string& context) {
    if (query_answer->handles[0] == query_answer->handles[1]) {
        LOG_DEBUG("Skipping because handles are the same: " + query_answer->handles[0]);
        return;
    }

    string predicate1 = query_answer->get(PREDICATE1);
    string predicate2 = query_answer->get(PREDICATE2);
    string concept1 = query_answer->get(CONCEPT1);

    LOG_DEBUG("Evaluating: " + get_predicate_name(predicate1) + " AND " +
              get_predicate_name(predicate2));
    string predicate_name = get_predicate_name(predicate1) + "_AND_" + get_predicate_name(predicate2);
    Link new_predicate = add_predicate(predicate_name);
    add_or_update_link(EVALUATION_HANDLE, new_predicate.handle(), concept1, 1.0);
}

static void build_equivalence_link(shared_ptr<QueryAnswer> query_answer, const string& context) {
    if (query_answer->handles[0] == query_answer->handles[1]) {
        LOG_DEBUG("Skipping because handles are the same: " + query_answer->handles[0]);
        return;
    }

    string concept_[2] = {query_answer->assignment.get(C1), query_answer->assignment.get(C2)};
    vector<vector<string>> query;
    for (unsigned int i = 0; i < 2; i++) {
        // clang-format off
        query.push_back({
            OR_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE1,
                ATOM, concept_[i],
            AND_OPERATOR, "2",
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, EVALUATION,
                    VARIABLE, PREDICATE2,
                    ATOM, concept_[i],
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, IMPLICATION,
                    VARIABLE, PREDICATE2,
                    VARIABLE, PREDICATE1,
        });
        // clang-format on
    }
    double count_0, count_1, count_intersection, count_union;
    QueryAnswerElement target_element(PREDICATE1);
    compute_counts(query,
                   context,
                   target_element,
                   concept_[0],
                   concept_[1],
                   count_0,
                   count_1,
                   count_intersection,
                   count_union);
    if ((count_intersection > 0) && (count_union > 0)) {
        double strength = count_intersection / count_union;
        add_or_update_link(EQUIVALENCE_HANDLE, concept_[0], concept_[1], strength);
        add_or_update_link(EQUIVALENCE_HANDLE, concept_[1], concept_[0], strength);
    }
}

static void build_links(const vector<string>& query,
                        const string& context,
                        void (*build_link)(shared_ptr<QueryAnswer> query_answer,
                                           const string& context)) {
    auto proxy = issue_link_building_query(query, context, LINK_BUILDING_QUERY_SIZE);
    unsigned int count = 0;
    shared_ptr<QueryAnswer> query_answer;
    while (!proxy->finished()) {
        if ((query_answer = proxy->pop()) == nullptr) {
            Utils::sleep(10000);
        } else {
            if (++count <= LINK_BUILDING_QUERY_SIZE) {
                if (query_answer->handles.size() == 2) {
                    LOG_DEBUG("Processing query answer " + to_string(count) + ": " +
                              query_answer->to_string());
                    build_link(query_answer, context);
                }
            } else {
                proxy->abort();
                break;
            }
        }
    }
}

// clang-format off
static void evolve_chain_query(
    const vector<string>& chain_query,
    const vector<vector<string>>& correlation_query_template,
    const vector<map<string, QueryAnswerElement>>& correlation_query_constants,
    const vector<pair<QueryAnswerElement, QueryAnswerElement>>& correlation_mapping,
    const string& context) {

    QueryEvolutionProxy* proxy_ptr = new QueryEvolutionProxy(
        chain_query,
        correlation_query_template,
        correlation_query_constants,
        correlation_mapping,
        context,
        FITNESS_FUNCTION);

    shared_ptr<QueryEvolutionProxy> proxy(proxy_ptr);
    proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] = false;
    proxy->parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] = false;
    proxy->parameters[QueryEvolutionProxy::POPULATION_SIZE] = (unsigned int) POPULATION_SIZE;
    proxy->parameters[QueryEvolutionProxy::MAX_GENERATIONS] = (unsigned int) MAX_GENERATIONS;
    proxy->parameters[QueryEvolutionProxy::ELITISM_RATE] = (double) ELITISM_RATE;
    proxy->parameters[QueryEvolutionProxy::SELECTION_RATE] = (double) SELECTION_RATE;
    proxy->parameters[QueryEvolutionProxy::TOTAL_ATTENTION_TOKENS] = (unsigned int) 100000;
    proxy->parameters[BaseQueryProxy::MAX_BUNDLE_SIZE] = (unsigned int) 1000;
    bus->issue_bus_command(proxy);

    shared_ptr<QueryAnswer> query_answer;
    unsigned int count_answers = 0;
    static double best_fitness = 0.0;
    static unsigned int count_iterations = 1;

    while (!proxy->finished()) {
        if ((query_answer = proxy->pop()) == NULL) {
            Utils::sleep();
        } else {
            count_answers++;
            if (query_answer->strength > best_fitness) {
                best_fitness = query_answer->strength;
                print_answer(query_answer);
            }
        }
    }
    cout << "Total answers in iteration " << count_iterations++ << ": " << count_answers << endl;
}
// clang-format on

static void run(const string& predicate_source,
                const string& predicate_target,
                const string& context_tag) {
    // clang-format off

    vector<string> implication_query = {
        AND_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, P1,
                VARIABLE, C,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, P2,
                VARIABLE, C,
    };

    vector<string> equivalence_query = {
        AND_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, P,
                VARIABLE, C1,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, P,
                VARIABLE, C2,
    };

    vector<string> and_predicate_query = {
        AND_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE1,
                VARIABLE, CONCEPT1,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE2,
                VARIABLE, CONCEPT1,
    };

    vector<string> chain_query = {
        OR_OPERATOR, "3",
            AND_OPERATOR, "2",
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, IMPLICATION,
                    ATOM, predicate_source,
                    VARIABLE, PREDICATE1,
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, IMPLICATION,
                    VARIABLE, PREDICATE1,
                    ATOM, predicate_target,
            AND_OPERATOR, "3",
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, IMPLICATION,
                    ATOM, predicate_source,
                    VARIABLE, PREDICATE1,
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, IMPLICATION,
                    VARIABLE, PREDICATE1,
                    VARIABLE, PREDICATE2,
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, IMPLICATION,
                    VARIABLE, PREDICATE2,
                    ATOM, predicate_target,
            AND_OPERATOR, "4",
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, IMPLICATION,
                    ATOM, predicate_source,
                    VARIABLE, PREDICATE1,
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, IMPLICATION,
                    VARIABLE, PREDICATE1,
                    VARIABLE, PREDICATE2,
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, IMPLICATION,
                    VARIABLE, PREDICATE2,
                    VARIABLE, PREDICATE3,
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, IMPLICATION,
                    VARIABLE, PREDICATE3,
                    ATOM, predicate_target,
    };

    vector<string> context_query = {
        OR_OPERATOR, "2",
        LINK_TEMPLATE, EXPRESSION, "3",
            NODE, SYMBOL, EVALUATION,
            VARIABLE, V2,
            VARIABLE, V3,
        LINK_TEMPLATE, EXPRESSION, "3",
            NODE, SYMBOL, CONTAINS,
            VARIABLE, V2,
            VARIABLE, V3,
    };

    // NOTE: The attention update query below is not used, but kept for reference.
    // vector<string> attention_update_query = {
    //     OR_OPERATOR, "2",
    //         AND_OPERATOR, "2",
    //             LINK_TEMPLATE, EXPRESSION, "3",
    //                 NODE, SYMBOL, EVALUATION,
    //                 ATOM, predicate_source,
    //                 LINK_TEMPLATE, EXPRESSION, "2",
    //                     NODE, SYMBOL, CONCEPT,
    //                     VARIABLE, SENTENCE1,
    //             LINK_TEMPLATE, EXPRESSION, "3",
    //                 NODE, SYMBOL, EVALUATION,
    //                 VARIABLE, PREDICATE1,
    //                 LINK_TEMPLATE, EXPRESSION, "2",
    //                     NODE, SYMBOL, CONCEPT,
    //                     VARIABLE, SENTENCE1,
    //         AND_OPERATOR, "2",
    //             LINK_TEMPLATE, EXPRESSION, "3",
    //                 NODE, SYMBOL, EVALUATION,
    //                 ATOM, predicate_target,
    //                 LINK_TEMPLATE, EXPRESSION, "2",
    //                     NODE, SYMBOL, CONCEPT,
    //                     VARIABLE, SENTENCE1,
    //             LINK_TEMPLATE, EXPRESSION, "3",
    //                 NODE, SYMBOL, EVALUATION,
    //                 VARIABLE, PREDICATE1,
    //                 LINK_TEMPLATE, EXPRESSION, "2",
    //                     NODE, SYMBOL, CONCEPT,
    //                     VARIABLE, SENTENCE1,
    // };

    vector<vector<string>> correlation_query_template = {{
        OR_OPERATOR, "3",
            AND_OPERATOR, "4",
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, EVALUATION,
                    VARIABLE, PREDICATE1,
                    LINK_TEMPLATE, EXPRESSION, "2",
                        NODE, SYMBOL, CONCEPT,
                        VARIABLE, SENTENCE1,
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, CONTAINS,
                    VARIABLE, SENTENCE1,
                    VARIABLE, WORD1,
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, CONTAINS,
                    VARIABLE, SENTENCE2,
                    VARIABLE, WORD1,
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, EVALUATION,
                    VARIABLE, PREDICATE2,
                    LINK_TEMPLATE, EXPRESSION, "2",
                        NODE, SYMBOL, CONCEPT,
                        VARIABLE, SENTENCE2,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, IMPLICATION,
                VARIABLE, PREDICATE1,
                VARIABLE, PREDICATE2,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, IMPLICATION,
                VARIABLE, PREDICATE2,
                VARIABLE, PREDICATE1,
    } /*, { TODO: requires revision to consider the AND as in the query above.
        OR_OPERATOR, "6"
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE1,
                VARIABLE, V1,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, IMPLICATION,
                VARIABLE, PREDICATE1,
                VARIABLE, V1,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, IMPLICATION,
                VARIABLE, V1,
                VARIABLE, PREDICATE1,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE2,
                VARIABLE, V1,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, IMPLICATION,
                VARIABLE, PREDICATE2,
                VARIABLE, V1,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, IMPLICATION,
                VARIABLE, V1,
                VARIABLE, PREDICATE2,
    } , {
        OR_OPERATOR, "9"
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE1,
                VARIABLE, V1,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, IMPLICATION,
                VARIABLE, PREDICATE1,
                VARIABLE, V1,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, IMPLICATION,
                VARIABLE, V1,
                VARIABLE, PREDICATE1,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE2,
                VARIABLE, V1,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, IMPLICATION,
                VARIABLE, PREDICATE2,
                VARIABLE, V1,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, IMPLICATION,
                VARIABLE, V1,
                VARIABLE, PREDICATE2,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE3,
                VARIABLE, V1,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, IMPLICATION,
                VARIABLE, PREDICATE3,
                VARIABLE, V1,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, IMPLICATION,
                VARIABLE, V1,
                VARIABLE, PREDICATE3,
    }*/};

    QueryAnswerElement predicate1(PREDICATE1);
    QueryAnswerElement predicate2(PREDICATE2);
    QueryAnswerElement predicate3(PREDICATE3);
    QueryAnswerElement sentence2(SENTENCE2);
    QueryAnswerElement v1(V1);

    vector<map<string, QueryAnswerElement>> correlation_query_constants = {
        {{PREDICATE1, predicate1}}/*,
        {{PREDICATE1, predicate1}, {PREDICATE2, predicate2}},
        {{PREDICATE1, predicate1}, {PREDICATE2, predicate2}, {PREDICATE3, predicate3}},*/
    };

    vector<vector<pair<QueryAnswerElement, QueryAnswerElement>>> correlation_mapping = {
        {{predicate1, predicate2}, {predicate1, sentence2}}/*,
        {{predicate1, v1}, {predicate2, v1}},
        {{predicate1, v1}, {predicate2, v1}, {predicate3, v1}},*/
    };

    /*******************************************************************
    Toplevel expressions (-> created -- knowledge base):

    ->  (IMPLICATION (PREDICATE "feature_contains_adc_cbc") (PREDICATE "feature_contains_ccc_cac"))
    ->  (EQUIVALENCE (CONCEPT (Sentence "dca cbc bde aaa adc") (CONCEPT (Sentence "aaa cac acc baa dad")))
    --  (EVALUATION (PREDICATE "feature_contains_adc_cbc") (CONCEPT (Sentence "dca cbc bde aaa adc")))
    --  (Contains (Sentence "cdb bce dad cec aae") (Word "bce"))

    Target elements (-> independent):

    ->  (PREDICATE "feature_contains_ccc_cac")
    ->  (CONCEPT (Sentence "dca cbc bde aaa adc"))
        (IMPLICATION (PREDICATE "feature_contains_adc_cbc") (PREDICATE "feature_contains_ccc_cac"))
        (EQUIVALENCE (CONCEPT (Sentence "dca cbc bde aaa adc")) (CONCEPT (Sentence "aaa cac acc baa dad")))
        (EVALUATION (PREDICATE "feature_contains_adc_cbc") (CONCEPT (Sentence "dca cbc bde aaa adc")))
    ********************************************************************/
    // clang-format on

    LOG_INFO("Source: " + get_predicate_name(predicate_source) +
             " Target: " + get_predicate_name(predicate_target));
    LOG_INFO("Setting up context");
    QueryAnswerElement target2(V2);
    QueryAnswerElement target3(V3);
    QueryAnswerElement toplevel_link(0);

    // For some reason, make_shared was not compiling so I'm explicitly creating the shared_ptr
    shared_ptr<ContextBrokerProxy> context_proxy(new ContextBrokerProxy(
        context_tag, context_query, {{toplevel_link, target2}, {toplevel_link, target3}}, {}));
    context_proxy->parameters[ContextBrokerProxy::USE_CACHE] = (bool) true;
    context_proxy->parameters[ContextBrokerProxy::ENFORCE_CACHE_RECREATION] = (bool) false;
    context_proxy->parameters[ContextBrokerProxy::INITIAL_RENT_RATE] = (double) RENT_RATE;
    context_proxy->parameters[ContextBrokerProxy::INITIAL_SPREADING_RATE_LOWERBOUND] =
        (double) SPREADING_RATE_LOWERBOUND;
    context_proxy->parameters[ContextBrokerProxy::INITIAL_SPREADING_RATE_UPPERBOUND] =
        (double) SPREADING_RATE_UPPERBOUND;
    // Issue the ContextBrokerProxy to create context
    ServiceBusSingleton::get_instance()->issue_bus_command(context_proxy);
    // Wait for ContextBrokerProxy to finish context creation
    LOG_INFO("Waiting for context creation to finish...");
    while (!context_proxy->is_context_created()) {
        Utils::sleep();
    }
    string context = context_proxy->get_key();
    LOG_INFO("Context " + context + " is ready");

    if (CONTEXT_CREATION_ONLY) {
        update_attention_allocation(predicate_source, predicate_target, context);
        Utils::sleep(5000);
        AttentionBrokerClient::save_context(context, CONTEXT_FILE_NAME);
        exit(0);
    } else {
        update_attention_allocation(predicate_source, predicate_target, context);
    }
    // update_context(predicate_source, predicate_target, context_proxy);

    for (unsigned int i = 0; i < NUM_ITERATIONS; i++) {
        LOG_INFO("--------------------------------------------------------------------------------");
        LOG_INFO("Iteration " + to_string(i + 1));
        LOG_INFO("--------------------------------------------------------------------------------");
        LOG_INFO("----- Building links");
        LOG_DEBUG("AND predicates");
        build_links(and_predicate_query, context, build_and_predicate_link);
        LOG_DEBUG("Equivalence");
        build_links(equivalence_query, context, build_equivalence_link);
        LOG_DEBUG("Implication");
        build_links(implication_query, context, build_implication_link);
        LOG_INFO("----- Updating AttentionBroker");
        AttentionBrokerClient::set_determiners(buffer_determiners, context);
        buffer_determiners.clear();
        LOG_INFO("----- Evolving query");
        evolve_chain_query(chain_query,
                           correlation_query_template,
                           correlation_query_constants,
                           correlation_mapping[0],
                           context);
    }
}

void insert_type_symbols() {
    vector<string> to_insert = {EQUIVALENCE, IMPLICATION};
    Node* node;
    for (string node_name : to_insert) {
        node = new Node(SYMBOL, node_name);
        if (!db->node_exists(node->handle())) {
            db->add_node(node);
        }
        delete (node);
    }
}

int main(int argc, char* argv[]) {
    // clang-format off
    if (argc != 15) {
        cerr << "Usage: " << argv[0]
             << " <client id> <server id> <start_port:end_port> <context_tag>"
                " <source_preEQUIVALENCE)dicate> <target_predicate>"
                " <RENT_RATE> <SPREADING_RATE_LOWERBOUND> <SPREADING_RATE_UPPERBOUND>"
                " <ELITISM_RATE> <SELECTION_RATE>"
                " <POPULATION_SIZE> <MAX_GENERATIONS> <NUM_ITERATIONS>" << endl;
        cerr << endl;
        cerr << "Suggested safe parameters:" << endl;
        cerr << "    RENT_RATE: 0.25" << endl;
        cerr << "    SPREADING_RATE_LOWERBOUND: 0.90" << endl;
        cerr << "    SPREADING_RATE_UPPERBOUND: 0.90" << endl;
        cerr << "    ELITISM_RATE: 0.08" << endl;
        cerr << "    SELECTION_RATE: 0.10" << endl;
        cerr << "    POPULATION_SIZE: 500" << endl;
        cerr << "    MAX_GENERATIONS: 20" << endl;
        cerr << "    NUM_ITERATIONS: 10" << endl;
        exit(1);
    }
    // clang-format on

    string client_id = argv[1];
    string server_id = argv[2];
    auto ports_range = Utils::parse_ports_range(argv[3]);
    string context_tag = argv[4];
    string source_predicate = argv[5];
    string target_predicate = argv[6];
    RENT_RATE = Utils::string_to_float(string(argv[7]));
    SPREADING_RATE_UPPERBOUND = Utils::string_to_float(string(argv[9]));
    ELITISM_RATE = (double) Utils::string_to_float(string(argv[10]));
    SELECTION_RATE = (double) Utils::string_to_float(string(argv[11]));
    POPULATION_SIZE = (unsigned int) Utils::string_to_int(string(argv[12]));
    MAX_GENERATIONS = (unsigned int) Utils::string_to_int(string(argv[13]));
    NUM_ITERATIONS = (unsigned int) Utils::string_to_int(string(argv[14]));

    AtomDBSingleton::init();
    db = AtomDBSingleton::get_instance();
    ServiceBusSingleton::init(client_id, server_id, ports_range.first, ports_range.second);
    FitnessFunctionRegistry::initialize_statics();
    bus = ServiceBusSingleton::get_instance();
    AttentionBrokerClient::set_parameters(
        RENT_RATE, SPREADING_RATE_LOWERBOUND, SPREADING_RATE_UPPERBOUND);
    insert_type_symbols();

    LOG_INFO("ELITISM_RATE: " + to_string(ELITISM_RATE));
    LOG_INFO("RENT_RATE: " + to_string(RENT_RATE));
    LOG_INFO("SPREADING_RATE_LOWERBOUND: " + to_string(SPREADING_RATE_LOWERBOUND));
    LOG_INFO("SPREADING_RATE_UPPERBOUND: " + to_string(SPREADING_RATE_UPPERBOUND));
    LOG_INFO("ELITISM_RATE: " + to_string(ELITISM_RATE));
    LOG_INFO("SELECTION_RATE: " + to_string(SELECTION_RATE));
    LOG_INFO("POPULATION_SIZE: " + to_string(POPULATION_SIZE));
    LOG_INFO("MAX_GENERATIONS: " + to_string(MAX_GENERATIONS));
    LOG_INFO("NUM_ITERATIONS: " + to_string(NUM_ITERATIONS));

    Node predicate_symbol(SYMBOL, PREDICATE);
    // cout << "XXXXX " << source_predicate << endl;
    // cout << "XXXXX " << target_predicate << endl;
    Node source_predicate_symbol(SYMBOL, "\"" + source_predicate + "\"");
    Node target_predicate_symbol(SYMBOL, "\"" + target_predicate + "\"");
    // cout << "XXXXX " << predicate_symbol.to_string() << " " << predicate_symbol.handle() << endl;
    // cout << "XXXXX " << source_predicate_symbol.to_string() << " " << source_predicate_symbol.handle()
    // << endl; cout << "XXXXX " << target_predicate_symbol.to_string() << " " <<
    // target_predicate_symbol.handle() << endl;
    Link source(EXPRESSION, {predicate_symbol.handle(), source_predicate_symbol.handle()});
    Link target(EXPRESSION, {predicate_symbol.handle(), target_predicate_symbol.handle()});
    // cout << "XXXXX " << source.to_string() << " " << source.handle() << endl;
    // cout << "XXXXX " << target.to_string() << " " << target.handle() << endl;
    run(source.handle(), target.handle(), context_tag);

    return 0;
}
