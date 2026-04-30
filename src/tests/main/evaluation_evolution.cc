#include <signal.h>

#define LOG_LEVEL INFO_LEVEL

#include <fstream>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>

#include "AtomDBAPITypes.h"
#include "AtomDBSingleton.h"
#include "AttentionBrokerClient.h"
#include "ContextBrokerProxy.h"
#include "CountLetterFunction.h"
#include "FitnessFunctionRegistry.h"
#include "Hasher.h"
#include "JsonConfigParser.h"
#include "Logger.h"
#include "MettaParser.h"
#include "PatternMatchingQueryProxy.h"
#include "QueryAnswer.h"
#include "QueryEvolutionProxy.h"
#include "ServiceBusSingleton.h"
#include "TestAtomDBJsonConfig.h"
#include "Utils.h"
#include "commons/atoms/MettaParserActions.h"

#define USE_MORK ((bool) true)

// Symbols
#define AND_OPERATOR "AND"
#define OR_OPERATOR "OR"
#define CHAIN_OPERATOR "CHAIN"
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
#define LOGICAL_AND "LogicalAnd"

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
static string LOGICAL_AND_HANDLE = Hasher::node_handle(SYMBOL, LOGICAL_AND);
static float RENT_RATE = 0.25;
static float SPREADING_RATE_LOWERBOUND = 0.90;
static float SPREADING_RATE_UPPERBOUND = 0.90;
static double SELECTION_RATE = 0.10;
static double ELITISM_RATE = 0.08;
static unsigned int LINK_BUILDING_QUERY_SIZE = 10;
static unsigned int POPULATION_SIZE = 50;
static unsigned int MAX_GENERATIONS = 20;
static unsigned int NUM_ITERATIONS = 10;

static string CONTEXT_FILE_NAME_PREFIX = "_CONTEXT_DUMP_";
static string NEW_LINKS_FILE_NAME = "newly_created_links.txt";
static bool WRITE_CREATED_LINKS_TO_DB = true;
static bool WRITE_CREATED_LINKS_TO_FILE = true;
static bool PRINT_CREATED_LINKS_METTA = true;

using namespace std;
using namespace atomdb;
using namespace commons;
using namespace query_engine;
using namespace evolution;
using namespace service_bus;
using namespace attention_broker;
using namespace context_broker;

enum ContextTaskType { UNDEFINED = 0, DETERMINER, CORRELATION, ACTIVATION };

static shared_ptr<AtomDB> db;
static shared_ptr<ServiceBus> bus;
static vector<vector<string>> buffer_determiners;
static map<string, vector<string>> weight_calculation_cache;

static string metta_expr3(const string& expr1, const string& expr2, const string& expr3) {
    return "(" + expr1 + " " + expr2 + " " + expr3 + ")";
}

static string metta_or(const string& expr1, const string& expr2) {
    return metta_expr3("or", expr1, expr2);
}

static string metta_and(const string& expr1, const string& expr2) {
    return metta_expr3("and", expr1, expr2);
}

static string metta_chain(const string& source, const string& target, const string& query) {
    return "(chain 0 1 2 " + source + " " + target + " " + query + ")";
}

static string metta_var(const string& name) { return "$" + name; }

static void save_link(Link& link) {
    ofstream file;
    file.open(NEW_LINKS_FILE_NAME, ios::app);
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
    proxy->parameters[BaseQueryProxy::ATTENTION_CORRELATION] = (unsigned int) BaseQueryProxy::NONE;
    proxy->parameters[BaseQueryProxy::ATTENTION_UPDATE] = (unsigned int) BaseQueryProxy::NONE;
    proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = true;
    proxy->parameters[BaseQueryProxy::USE_LINK_TEMPLATE_CACHE] = false;
    proxy->parameters[PatternMatchingQueryProxy::MAX_ANSWERS] = (unsigned int) max_answers;
    proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = (max_answers != 0);
    proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] = false;
    proxy->parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] = USE_MORK;
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
    proxy->parameters[BaseQueryProxy::ATTENTION_CORRELATION] = (unsigned int) BaseQueryProxy::NONE;
    proxy->parameters[BaseQueryProxy::ATTENTION_UPDATE] = (unsigned int) BaseQueryProxy::NONE;
    proxy->parameters[BaseQueryProxy::USE_LINK_TEMPLATE_CACHE] = false;
    proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = false;
    proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] = USE_MORK;
    proxy->parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] = false;

    ServiceBusSingleton::get_instance()->issue_bus_command(proxy);
    return proxy;
}

static shared_ptr<PatternMatchingQueryProxy> issue_context_creation_query(
    const vector<string>& query_tokens, const string& context) {
    auto proxy = make_shared<PatternMatchingQueryProxy>(query_tokens, context);
    proxy->parameters[BaseQueryProxy::ATTENTION_CORRELATION] = (unsigned int) BaseQueryProxy::NONE;
    proxy->parameters[BaseQueryProxy::ATTENTION_UPDATE] = (unsigned int) BaseQueryProxy::NONE;
    proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = true;
    proxy->parameters[BaseQueryProxy::USE_LINK_TEMPLATE_CACHE] = false;
    proxy->parameters[PatternMatchingQueryProxy::MAX_ANSWERS] = (unsigned int) 0;
    proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = false;
    proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] = (query_tokens.size() == 1);
    proxy->parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] = false;
    proxy->parameters[PatternMatchingQueryProxy::UNIQUE_VALUE_FLAG] = true;

    ServiceBusSingleton::get_instance()->issue_bus_command(proxy);
    return proxy;
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
        while (!proxy[i]->finished()) {
            if ((query_answer = proxy[i]->pop()) == NULL) {
                Utils::sleep();
            } else {
                if (query_answer->get_handles_size() == 1) {
                    d = 1.0;
                } else {
                    auto link = db->get_atom(query_answer->get(1));
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
    LOG_LOCAL_DEBUG("Returning from add_or_update_link()");
    return new_link;
}

static Link add_predicate(const string& handle1, const string& handle2) {
    string h1, h2;
    if (handle1 < handle2) {
        h1 = handle1;
        h2 = handle2;
    } else {
        h1 = handle2;
        h2 = handle1;
    }
    Link new_link(EXPRESSION, {LOGICAL_AND_HANDLE, h1, h2}, false);
    if (!db->link_exists(new_link.handle())) {
        db->add_link(&new_link);
    }
    return new_link;
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
    set<string> ab_request = {new_predicate.handle(), concept1};
    AttentionBrokerClient::correlate(ab_request, context);
}

static void build_implication_link(shared_ptr<QueryAnswer> query_answer,
                                   const string& context,
                                   const string& custom_handle) {
    string predicates[2];
    string metta_predicates[2];
    if (custom_handle == "") {
        // build Evaluation of the AND of both predicates
        build_and_predicate_link(query_answer, context, custom_handle);
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

    metta_predicates[0] = query_answer->metta_expression[predicates[0]];
    metta_predicates[1] = query_answer->metta_expression[predicates[1]];

    vector<vector<string>> query;
    for (unsigned int i = 0; i < 2; i++) {
        // clang-format off
        if (USE_MORK) {
            vector<string> metta_query = {
                metta_or(
                    metta_expr3(EVALUATION, metta_predicates[i], metta_var(CONCEPT1)), 
                    metta_and(
                        metta_expr3(EVALUATION, metta_predicates[i], metta_var(CONCEPT2)),
                        metta_expr3(EQUIVALENCE, metta_var(CONCEPT2), metta_var(CONCEPT1))))};
            LOG_DEBUG("Counting query (implication): " + metta_query[0]);
            query.push_back(metta_query);
        } else {
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
        }
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
        bool link_created = false;
        if (count_0 > 0) {
            double strength = count_intersection / count_0;
            add_or_update_link(IMPLICATION_HANDLE, predicates[0], predicates[1], strength);
            link_created = true;
        }
        if (count_1 > 0) {
            double strength = count_intersection / count_1;
            add_or_update_link(IMPLICATION_HANDLE, predicates[1], predicates[0], strength);
            link_created = true;
        }
        if (link_created) {
            set<string> ab_request = {predicates[0], predicates[1]};
            AttentionBrokerClient::correlate(ab_request, context);
        }
    }
}

static void build_equivalence_link(shared_ptr<QueryAnswer> query_answer,
                                   const string& context,
                                   const string& custom_handle) {
    string concepts[2];
    string metta_concepts[2];
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

    metta_concepts[0] = query_answer->metta_expression[concepts[0]];
    metta_concepts[1] = query_answer->metta_expression[concepts[1]];

    vector<vector<string>> query;
    for (unsigned int i = 0; i < 2; i++) {
        // clang-format off
        if (USE_MORK) {
            vector<string> metta_query = {
                metta_or(
                    metta_expr3(EVALUATION, metta_var(PREDICATE1), metta_concepts[i]), 
                    metta_and(
                        metta_expr3(EVALUATION, metta_var(PREDICATE2), metta_concepts[i]), 
                        metta_expr3(IMPLICATION, metta_var(PREDICATE2), metta_var(PREDICATE1))))};
            LOG_DEBUG("Counting query (equivalence): " + metta_query[0]);
            query.push_back(metta_query);
        } else {
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
        }
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
        set<string> ab_request = {concepts[0], concepts[1]};
        AttentionBrokerClient::correlate(ab_request, context);
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
                if (query_answer->get_handles_size() == 2) {
                    LOG_DEBUG("Processing query answer " + to_string(count) + ": " +
                              query_answer->to_string(true));
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
    unsigned int answer_arity = query_answer->get_handles_size();
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
    proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] = USE_MORK;
    proxy->parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] = false;
    proxy->parameters[QueryEvolutionProxy::POPULATION_SIZE] = (unsigned int) POPULATION_SIZE;
    proxy->parameters[QueryEvolutionProxy::MAX_GENERATIONS] = (unsigned int) MAX_GENERATIONS;
    proxy->parameters[QueryEvolutionProxy::ELITISM_RATE] = (double) ELITISM_RATE;
    proxy->parameters[QueryEvolutionProxy::SELECTION_RATE] = (double) SELECTION_RATE;
    proxy->parameters[QueryEvolutionProxy::TOTAL_ATTENTION_TOKENS] = (unsigned int) 100000;
    proxy->parameters[BaseQueryProxy::MAX_BUNDLE_SIZE] = (unsigned int) 1000;
    proxy->parameters[BaseQueryProxy::ALLOW_INCOMPLETE_CHAIN_PATH] = true;
    proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = true;
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

static void add_to_context_file(const filesystem::path& context_file_name, 
                                const string& context, 
                                ContextTaskType task, 
                                const vector<string>& query, 
                                const vector<vector<QueryAnswerElement>>& selector) {

    ofstream file;
    file.open(context_file_name, ios::app);
    string mnemonic = "";
    if (file.is_open()) {
        auto proxy = issue_context_creation_query(query, context);
        unsigned int count = 0;
        shared_ptr<QueryAnswer> query_answer;
        while (!proxy->finished()) {
            if ((query_answer = proxy->pop()) == nullptr) {
                Utils::sleep();
            } else {
                count++;
                switch(task) {
                    case DETERMINER:
                        mnemonic = "DET";
                    case CORRELATION:
                        if (mnemonic == "") mnemonic = "COR";
                        for (auto pair : selector) {
                            if (pair.size() != 2) {
                                Utils::error("Invalid context task selector for " + mnemonic);
                                return;
                            }
                            file << mnemonic << " " << query_answer->get(pair[0]) << " " << query_answer->get(pair[1]) << endl;
                        }
                        break;
                    case ACTIVATION:
                        mnemonic = "ACT";
                        if (selector.size() != 1) {
                            Utils::error("Invalid context task selector for " + mnemonic);
                        }
                        file << mnemonic;
                        for (auto element : selector[0]) {
                            file << " " << query_answer->get(element);
                        }
                        file << endl;
                        break;
                    default: Utils::error("Invalid context creation task: " + std::to_string((unsigned int) task));
                }
            }
        }
        file.close();
    } else {
        Utils::error("Couldn't open file for writing: " + string(context_file_name));
    }
}

static void run(const string& target_predicate,
                const string& target_concept,
                const string& target_predicate_handle,
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

    vector<string> query_to_evolve = {
        AND_OPERATOR, "3",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE,
                VARIABLE, CONCEPT,
            CHAIN_OPERATOR, "0", "1", "2",
                VARIABLE, CONCEPT,
                ATOM, target_concept_handle,
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, EQUIVALENCE,
                    VARIABLE, CONCEPT1,
                    VARIABLE, CONCEPT2,
            CHAIN_OPERATOR, "0", "1", "2",
                VARIABLE, PREDICATE,
                ATOM, target_predicate_handle,
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, IMPLICATION,
                    VARIABLE, PREDICATE1,
                    VARIABLE, PREDICATE2,
    };
    vector<string> metta_query_to_evolve = {
        metta_and(
            metta_and(
                metta_expr3(EVALUATION, metta_var(PREDICATE), metta_var(CONCEPT)),
                metta_chain(metta_var(CONCEPT), target_concept, metta_expr3(EQUIVALENCE, metta_var(CONCEPT1) , metta_var(CONCEPT2)))),
            metta_chain(metta_var(PREDICATE), target_predicate, metta_expr3(IMPLICATION, metta_var(PREDICATE1) , metta_var(PREDICATE2))))
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

    vector<string> context_determiner_query = {
        LINK_TEMPLATE, EXPRESSION, "3",
            NODE, SYMBOL, EVALUATION,
            VARIABLE, PREDICATE,
            VARIABLE, CONCEPT,
    };

    vector<string> context_correlation_query = {
        LINK_TEMPLATE, EXPRESSION, "3",
            NODE, SYMBOL, EVALUATION,
            VARIABLE, PREDICATE,
            VARIABLE, CONCEPT,
    };

    vector<string> context_activation_query1 = {
        LINK_TEMPLATE, EXPRESSION, "3",
            NODE, SYMBOL, EVALUATION,
            ATOM, target_predicate_handle,
            VARIABLE, CONCEPT,
    };
    vector<string> context_activation_metta_query1 = {
        metta_expr3(EVALUATION, target_predicate, metta_var(CONCEPT))
    };

    vector<string> context_activation_query2 = {
        LINK_TEMPLATE, EXPRESSION, "3",
            NODE, SYMBOL, EVALUATION,
            VARIABLE, PREDICATE,
            ATOM, target_concept_handle,
    };
    vector<string> context_activation_metta_query2 = {
        metta_expr3(EVALUATION, metta_var(PREDICATE), target_concept)
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
    string context = Hasher::context_handle(context_tag);
    filesystem::path context_file_name = CONTEXT_FILE_NAME_PREFIX + context + ".txt";
    if (! filesystem::exists(context_file_name)) {
        LOG_INFO("Context file doesn't exist. Creating it...");
        QueryAnswerElement qe_predicate(PREDICATE);
        QueryAnswerElement qe_concept(CONCEPT);
        QueryAnswerElement qe_toplevel(0);
        LOG_INFO("Creating determiners");
        add_to_context_file(context_file_name, context, DETERMINER, context_determiner_query, {{qe_toplevel, qe_predicate}, {qe_toplevel, qe_concept}});
        LOG_INFO("Making correlations");
        add_to_context_file(context_file_name, context, CORRELATION, context_correlation_query, {{qe_concept, qe_predicate}, {qe_predicate, qe_concept}});
        LOG_INFO("Spreading activation");
        add_to_context_file(context_file_name, context, ACTIVATION, (USE_MORK ? context_activation_metta_query1 : context_activation_query1), {{qe_concept}});
        add_to_context_file(context_file_name, context, ACTIVATION, (USE_MORK ? context_activation_metta_query2 : context_activation_query2), {{qe_predicate}});
    } else {
        LOG_INFO("Context file already exists. Reusing it...");
    }
    LOG_INFO("Updating AttentionBroker");
    return;
    //read_context(context, context_file_name);
    LOG_INFO("Context " + context + " is ready");

    /*
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
    LOG_INFO("Context " + context + " is ready");
    */

    // LOG_INFO("Pre-processing...");
    // LOG_INFO("Make initial correlations");
    // attention_correlation_query(initialization_correlation_query, context);
    // LOG_INFO("Initializing STI");
    // attention_allocation_query((USE_MORK ? initialization_STI_metta_query : initialization_STI_query), context);
    // LOG_INFO("Building initial custom links");
    // build_links(
    //     custom_initial_equivalence_query, context, 0, target_concept_handle, build_equivalence_link);
    // build_links(
    //     custom_initial_implication_query, context, 0, target_predicate_handle,
    //     build_implication_link);
    // LOG_INFO("Pre-processing complete");

    for (unsigned int i = 0; i < NUM_ITERATIONS; i++) {
        LOG_INFO("--------------------------------------------------------------------------------");
        LOG_INFO("Iteration " + to_string(i + 1));
        LOG_INFO("--------------------------------------------------------------------------------");
        LOG_INFO("----- Building links");
        LOG_INFO("Building Implication links");
        build_links(implication_query, context, LINK_BUILDING_QUERY_SIZE, "", build_implication_link);
        LOG_INFO("Building Equivalence links");
        build_links(equivalence_query, context, LINK_BUILDING_QUERY_SIZE, "", build_equivalence_link);
        LOG_INFO("----- Updating AttentionBroker");
        AttentionBrokerClient::set_determiners(buffer_determiners, context);
        buffer_determiners.clear();
        LOG_INFO("----- Evolving query");
        query_evolution((USE_MORK ? metta_query_to_evolve : query_to_evolve),
                        correlation_query_template,
                        correlation_query_constants,
                        correlation_mapping[0],
                        context);
    }
}

void insert_type_symbols() {
    vector<string> to_insert = {EQUIVALENCE, IMPLICATION, LOGICAL_AND};
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
    if (argc != 16) {
        cerr << "Usage: " << argv[0]
             << " <client_endpoint> <server_endpoint> <start_port:end_port> <config_file>"
                " <context_tag> <target_predicate> <target_concept>"
                " <RENT_RATE> <SPREADING_RATE_LOWERBOUND> <SPREADING_RATE_UPPERBOUND>"
                " <ELITISM_RATE> <SELECTION_RATE> <POPULATION_SIZE> <MAX_GENERATIONS> <NUM_ITERATIONS>" << endl;
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
    string config_file = argv[++cursor];
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

    if (cursor != 15) {
        Utils::error("Error setting up parameters");
    }

    auto json_config = JsonConfigParser::load(config_file);
    auto atomdb_config = json_config.at_path("atomdb").get_or<JsonConfig>(JsonConfig());
    AtomDBSingleton::init(atomdb_config);

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

    run(target_predicate,
        target_concept,
        predicate_pa->metta_expression_handle,
        concept_pa->metta_expression_handle,
        context_tag);

    return 0;
}
