#include <signal.h>

#define LOG_LEVEL INFO_LEVEL

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "AtomSpace.h"
#include "AttentionBrokerClient.h"
#include "Context.h"
#include "ContextBrokerProxy.h"
#include "CountLetterFunction.h"
#include "FitnessFunctionRegistry.h"
#include "Hasher.h"
#include "Logger.h"
#include "MettaParser.h"
#include "QueryAnswer.h"
#include "QueryEvolutionProxy.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"
#include "commons/atoms/MettaParserActions.h"

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
#define V1 "V1"
#define V2 "V2"
#define V3 "V3"
#define PREDICATE1 "Predicate1"
#define PREDICATE2 "Predicate2"
#define PREDICATE3 "Predicate3"
#define CONCEPT1 "Concept1"
#define CONCEPT2 "Concept2"
#define CONCEPT3 "Concept3"

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
static unsigned int LINK_BUILDING_QUERY_SIZE = 50;
static unsigned int POPULATION_SIZE = 50;
static unsigned int MAX_GENERATIONS = 20;
static unsigned int NUM_ITERATIONS = 10;

static string CONTEXT_FILE_NAME = "_CONTEXT_DUMP";
static string NEW_LINKS_FILE_NAME = "newly_created_links.txt";
static bool WRITE_CREATED_LINKS_TO_DB = true;
static bool WRITE_CREATED_LINKS_TO_FILE = true;
static bool PRINT_CREATED_LINKS_METTA = true;

using namespace std;
using namespace atomdb;
using namespace atom_space;
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

static shared_ptr<PatternMatchingQueryProxy> issue_link_building_query(
    const vector<string>& query_tokens, const string& context, unsigned int max_answers) {
    auto proxy = make_shared<PatternMatchingQueryProxy>(query_tokens, context);
    proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = true;
    proxy->parameters[BaseQueryProxy::ATTENTION_UPDATE_FLAG] = false;
    // proxy->parameters[BaseQueryProxy::USE_LINK_TEMPLATE_CACHE] = true; // Use the default value
    proxy->parameters[PatternMatchingQueryProxy::MAX_ANSWERS] = (unsigned int) max_answers;
    proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = (max_answers != 0);
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

static void attention_allocation_query(const vector<string>& query_tokens, const string& context) {
    auto proxy = make_shared<PatternMatchingQueryProxy>(query_tokens, context);
    proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = true;
    proxy->parameters[BaseQueryProxy::ATTENTION_UPDATE_FLAG] = true;
    proxy->parameters[BaseQueryProxy::USE_LINK_TEMPLATE_CACHE] = false;
    proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = false;
    proxy->parameters[PatternMatchingQueryProxy::COUNT_FLAG] = true;
    proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] = false;
    proxy->parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] = false;

    ServiceBusSingleton::get_instance()->issue_bus_command(proxy);
    while (!proxy->finished()) {
        Utils::sleep();
    }
}

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
    if (PRINT_CREATED_LINKS_METTA) {
        LOG_INFO("ADD LINK: " +
                 new_link.metta_representation(*static_pointer_cast<HandleDecoder>(db).get()));
    }
    string handle = new_link.handle();
    if (db->link_exists(handle)) {
        auto old_link = db->get_atom(handle);
        LOG_DEBUG("Link already exists: " + old_link->to_string());
        if (strength > old_link->custom_attributes.get<double>(STRENGTH)) {
            if (WRITE_CREATED_LINKS_TO_DB) {
                LOG_DEBUG("Updating Link in AtomDB");
                db->delete_link(handle, false);
                db->add_link(&new_link);
            }
            if (WRITE_CREATED_LINKS_TO_FILE) {
                LOG_DEBUG("Writing Link to file: " + NEW_LINKS_FILE_NAME);
                save_link(new_link);
            }
        }
    } else {
        if (WRITE_CREATED_LINKS_TO_DB) {
            LOG_DEBUG("Creating Link in AtomDB");
            db->add_link(&new_link);
        }
        buffer_determiners.push_back({handle, target1, target2});
        if (WRITE_CREATED_LINKS_TO_FILE) {
            LOG_DEBUG("Writing Link to file: " + NEW_LINKS_FILE_NAME);
            save_link(new_link);
        }
    }
    return new_link;
}

static Link add_predicate(const string& handle1, const string& handle2) {
    Link new_link(EXPRESSION, {handle1, handle2}, false);
    if (!db->link_exists(new_link.handle())) {
        db->add_link(&new_link);
    }
    return new_link;
}

static void build_implication_link(shared_ptr<QueryAnswer> query_answer,
                                   const string& context,
                                   const string& custom_handle) {
    string predicates[2];
    if (custom_handle == "") {
        predicates[0] = query_answer->get(PREDICATE1);
        predicates[1] = query_answer->get(PREDICATE2);
    } else {
        predicates[0] = custom_handle;
        predicates[1] = query_answer->get(0);
    }

    if (predicates[0] == predicates[1]) {
        LOG_DEBUG("Skipping link building because targets are the same: " + predicates[0]);
        return;
    }

    vector<vector<string>> query;
    for (unsigned int i = 0; i < 2; i++) {
        // clang-format off
        query.push_back({
            OR_OPERATOR, "2",
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, EVALUATION,
                    ATOM, predicates[i],
                    VARIABLE, CONCEPT1,
                AND_OPERATOR, "2",
                    LINK_TEMPLATE, EXPRESSION, "3",
                        NODE, SYMBOL, EVALUATION,
                        ATOM, predicates[i],
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
                   predicates[0],
                   predicates[1],
                   count_0,
                   count_1,
                   count_intersection,
                   count_union);
    if (count_intersection > 0) {
        if (count_0 > 0) {
            double strength = count_intersection / count_0;
            add_or_update_link(IMPLICATION_HANDLE, predicates[0], predicates[1], strength);
        }
        if (count_1 > 0) {
            double strength = count_intersection / count_1;
            add_or_update_link(IMPLICATION_HANDLE, predicates[1], predicates[0], strength);
        }
    }
}

static void build_and_predicate_link(shared_ptr<QueryAnswer> query_answer,
                                     const string& context,
                                     const string& custom_handle) {
    string predicate1 = query_answer->get(PREDICATE1);
    string predicate2 = query_answer->get(PREDICATE2);
    string concept1 = query_answer->get(CONCEPT);

    if (predicate1 == predicate2) {
        LOG_DEBUG("Skipping link building because targets are the same: " + predicate1);
        return;
    }

    Link new_predicate = add_predicate(predicate1, predicate2);
    add_or_update_link(EVALUATION_HANDLE, new_predicate.handle(), concept1, 1.0);
}

static void build_equivalence_link(shared_ptr<QueryAnswer> query_answer,
                                   const string& context,
                                   const string& custom_handle) {
    string concepts[2];
    if (custom_handle == "") {
        concepts[0] = query_answer->get(CONCEPT1);
        concepts[1] = query_answer->get(CONCEPT2);
    } else {
        concepts[0] = custom_handle;
        concepts[1] = query_answer->get(0);
    }

    if (concepts[0] == concepts[1]) {
        LOG_DEBUG("Skipping link building because targets are the same: " + concepts[0]);
        return;
    }

    vector<vector<string>> query;
    for (unsigned int i = 0; i < 2; i++) {
        // clang-format off
        query.push_back({
            OR_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE1,
                ATOM, concepts[i],
            AND_OPERATOR, "2",
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, EVALUATION,
                    VARIABLE, PREDICATE2,
                    ATOM, concepts[i],
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
                   concepts[0],
                   concepts[1],
                   count_0,
                   count_1,
                   count_intersection,
                   count_union);
    if ((count_intersection > 0) && (count_union > 0)) {
        double strength = count_intersection / count_union;
        add_or_update_link(EQUIVALENCE_HANDLE, concepts[0], concepts[1], strength);
        add_or_update_link(EQUIVALENCE_HANDLE, concepts[1], concepts[0], strength);
    }
}

static void build_links(const vector<string>& query,
                        const string& context,
                        unsigned int num_links,
                        const string& custom_handle,
                        void (*build_link)(shared_ptr<QueryAnswer> query_answer,
                                           const string& context,
                                           const string& custom_handle)) {
    auto proxy = issue_link_building_query(query, context, num_links);
    unsigned int count = 0;
    shared_ptr<QueryAnswer> query_answer;
    while (!proxy->finished()) {
        if ((query_answer = proxy->pop()) == nullptr) {
            Utils::sleep();
        } else {
            if (++count <= num_links) {
                if (query_answer->handles.size() == 2) {
                    LOG_DEBUG("Processing query answer " + to_string(count) + ": " +
                              query_answer->to_string());
                    build_link(query_answer, context, custom_handle);
                }
            } else {
                Utils::sleep();
                if (!proxy->finished()) {
                    proxy->abort();
                }
                break;
            }
        }
    }
}

static void print_answer(shared_ptr<QueryAnswer> query_answer) {
    unsigned int answer_arity = query_answer->handles.size();
    cout << fixed << setw(6) << setprecision(4) << setfill('0');
    cout << query_answer->strength << " [" << to_string(answer_arity) << "]";
    cout << endl;
}

// clang-format off
static void query_evolution(
    const vector<string>& query_to_evolve,
    const vector<vector<string>>& correlation_query_template,
    const vector<map<string, QueryAnswerElement>>& correlation_query_constants,
    const vector<pair<QueryAnswerElement, QueryAnswerElement>>& correlation_mapping,
    const string& context) {

    QueryEvolutionProxy* proxy_ptr = new QueryEvolutionProxy(
        query_to_evolve,
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

static void run(const string& target_predicate_handle,
                const string& target_concept_handle,
                const string& context_tag) {
    // clang-format off

    vector<string> implication_query = {
        AND_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE1,
                VARIABLE, CONCEPT,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE2,
                VARIABLE, CONCEPT,
    };

    vector<string> equivalence_query = {
        AND_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE,
                VARIABLE, CONCEPT1,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE,
                VARIABLE, CONCEPT2,
    };

    vector<string> and_predicate_query = {
        AND_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE1,
                VARIABLE, CONCEPT,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE2,
                VARIABLE, CONCEPT,
    };

    vector<string> query_to_evolve = {
        OR_OPERATOR, "3",
            AND_OPERATOR, "2",
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, EVALUATION,
                    ATOM, target_predicate_handle,
                    VARIABLE, CONCEPT,
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, EQUIVALENCE,
                    VARIABLE, CONCEPT,
                    ATOM, target_concept_handle,
            AND_OPERATOR, "2",
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, EVALUATION,
                    VARIABLE, PREDICATE,
                    ATOM, target_concept_handle,
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, IMPLICATION,
                    VARIABLE, PREDICATE,
                    ATOM, target_predicate_handle,
            AND_OPERATOR, "3",
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, EVALUATION,
                    VARIABLE, PREDICATE,
                    VARIABLE, CONCEPT,
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, EQUIVALENCE,
                    VARIABLE, CONCEPT,
                    ATOM, target_concept_handle,
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, IMPLICATION,
                    VARIABLE, PREDICATE,
                    ATOM, target_predicate_handle,
    };

    vector<vector<string>> correlation_query_template = {{
        OR_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, V1,
                VARIABLE, CONCEPT,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE,
                VARIABLE, V2,
    }};

    vector<string> context_query = {
        LINK_TEMPLATE, EXPRESSION, "3",
            NODE, SYMBOL, EVALUATION,
            VARIABLE, V2,
            VARIABLE, V3,
    };

    vector<string> initialization_STI_query = {
        OR_OPERATOR, "2",
        LINK_TEMPLATE, EXPRESSION, "3",
            NODE, SYMBOL, EVALUATION,
            ATOM, target_predicate_handle,
            VARIABLE, CONCEPT,
        LINK_TEMPLATE, EXPRESSION, "3",
            NODE, SYMBOL, EVALUATION,
            VARIABLE, PREDICATE,
            ATOM, target_concept_handle,
    };

    vector<string> custom_initial_equivalence_query = {
        LINK_TEMPLATE, EXPRESSION, "2",
            NODE, SYMBOL, CONCEPT,
            VARIABLE, V1,
    };

    vector<string> custom_initial_implication_query = {
        LINK_TEMPLATE, EXPRESSION, "2",
            NODE, SYMBOL, PREDICATE,
            VARIABLE, V1,
    };

    QueryAnswerElement qa_predicate(PREDICATE);
    QueryAnswerElement qa_concept(CONCEPT);

    vector<map<string, QueryAnswerElement>> correlation_query_constants = {
        {{V1, qa_predicate}, {V2, qa_concept}}
    };

    vector<vector<pair<QueryAnswerElement, QueryAnswerElement>>> correlation_mapping = {
        {{qa_predicate, qa_predicate}, {qa_concept, qa_concept}}
    };

    // clang-format on

    LOG_INFO("Setting up context for tag: " + context_tag);
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

    LOG_INFO("Pre-processing...");
    LOG_INFO("Initializing STI");
    attention_allocation_query(initialization_STI_query, context);
    //LOG_INFO("Building initial custom links");
    //build_links(
    //    custom_initial_equivalence_query, context, 0, target_concept_handle, build_equivalence_link);
    //build_links(
    //    custom_initial_implication_query, context, 0, target_predicate_handle, build_implication_link);
    LOG_INFO("Pre-processing complete");

    for (unsigned int i = 0; i < NUM_ITERATIONS; i++) {
        LOG_INFO("--------------------------------------------------------------------------------");
        LOG_INFO("Iteration " + to_string(i + 1));
        LOG_INFO("--------------------------------------------------------------------------------");
        LOG_INFO("----- Building links");
        LOG_INFO("Building AND predicates");
        build_links(
            and_predicate_query, context, LINK_BUILDING_QUERY_SIZE, "", build_and_predicate_link);
        LOG_INFO("Building Equivalence links");
        build_links(equivalence_query, context, LINK_BUILDING_QUERY_SIZE, "", build_equivalence_link);
        LOG_INFO("Building Implication links");
        build_links(implication_query, context, LINK_BUILDING_QUERY_SIZE, "", build_implication_link);
        LOG_INFO("----- Updating AttentionBroker");
        AttentionBrokerClient::set_determiners(buffer_determiners, context);
        buffer_determiners.clear();
        LOG_INFO("----- Evolving query");
        query_evolution(query_to_evolve,
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
    if (argc < 15) {
        cerr << "Usage: " << argv[0]
             << " <client_endpoint> <server_endpoint> <start_port:end_port>"
                " <context_tag> <target_predicate> <target_concept>"
                " <RENT_RATE> <SPREADING_RATE_LOWERBOUND> <SPREADING_RATE_UPPERBOUND>"
                " <ELITISM_RATE> <SELECTION_RATE> <POPULATION_SIZE> <MAX_GENERATIONS> <NUM_ITERATIONS> [--use-mork]" << endl;
        cerr << endl;
        cerr << "<target_predicate> <target_concept> are MeTTa expressions" << endl;
        cerr << endl;
        cerr << endl;
        cerr << "Suggested safe parameters:" << endl;
        cerr << endl;
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

    int cursor = 0;

    string client_endpoint = argv[++cursor];
    string server_endpoint = argv[++cursor];
    auto ports_range = Utils::parse_ports_range(argv[++cursor]);

    string context_tag = argv[++cursor];
    string target_predicate = argv[++cursor];
    string target_concept = argv[++cursor];

    RENT_RATE = Utils::string_to_float(string(argv[++cursor]));
    SPREADING_RATE_LOWERBOUND = Utils::string_to_float(string(argv[++cursor]));
    SPREADING_RATE_UPPERBOUND = Utils::string_to_float(string(argv[++cursor]));

    ELITISM_RATE = (double) Utils::string_to_float(string(argv[++cursor]));
    SELECTION_RATE = (double) Utils::string_to_float(string(argv[++cursor]));
    POPULATION_SIZE = (unsigned int) Utils::string_to_int(string(argv[++cursor]));
    MAX_GENERATIONS = (unsigned int) Utils::string_to_int(string(argv[++cursor]));
    NUM_ITERATIONS = (unsigned int) Utils::string_to_int(string(argv[++cursor]));

    if (cursor != 14) {
        Utils::error("Error setting up parameters");
    }

    if ((++cursor < argc) && (string(argv[cursor]) == string("--use-mork"))) {
        AtomDBSingleton::init(atomdb_api_types::ATOMDB_TYPE::MORKDB);
    } else {
        AtomDBSingleton::init();
    }

    db = AtomDBSingleton::get_instance();
    ServiceBusSingleton::init(client_endpoint, server_endpoint, ports_range.first, ports_range.second);
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

    shared_ptr<atoms::MettaParserActions> predicate_pa = make_shared<atoms::MettaParserActions>();
    shared_ptr<atoms::MettaParserActions> concept_pa = make_shared<atoms::MettaParserActions>();
    MettaParser predicate_p(target_predicate, predicate_pa);
    MettaParser concept_p(target_concept, concept_pa);
    predicate_p.parse();
    concept_p.parse();

    LOG_INFO("Target predicate: " + target_predicate +
             " Handle: " + predicate_pa->metta_expression_handle);
    LOG_INFO("Target concept: " + target_concept + " Handle: " + concept_pa->metta_expression_handle);

    run(predicate_pa->metta_expression_handle, concept_pa->metta_expression_handle, context_tag);

    return 0;
}
