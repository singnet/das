#include <signal.h>

#include <filesystem>
#include <fstream>
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
#include "SystemParametersSingleton.h"
#include "TestAtomDBJsonConfig.h"
#include "Utils.h"
#include "commons/atoms/MettaParserActions.h"

// Symbols
#define AND_OPERATOR "AND"
#define ANDNOT_OPERATOR "ANDNOT"
#define OR_OPERATOR "OR"
#define CHAIN_OPERATOR "CHAIN"
#define LINK_TEMPLATE "LINK_TEMPLATE"
#define LINK "LINK"
#define NODE "NODE"
#define VARIABLE "VARIABLE"
#define ATOM "ATOM"
#define EXPRESSION "Expression"
#define SYMBOL "Symbol"
#define EVALUATION "Evaluation"
#define CONCEPT "Concept"
#define PREDICATE "Predicate"
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
#define STRENGTH_TAG "strength"
#define IS_LITERAL "is_literal"
#define FITNESS_FUNCTION "inference_toy"

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
static unsigned int POPULATION_SIZE = 50;
static unsigned int MAX_GENERATIONS = 20;
static unsigned int NUM_ITERATIONS = 10;

static string TARGET_CONCEPT = "undefined";
static string TARGET_PREDICATE = "undefined";
static string TARGET_CONCEPT_HANDLE = "undefined";
static string TARGET_PREDICATE_HANDLE = "undefined";

static bool USE_MORK = false;
static bool SETUP_ONLY = false;
static double LINK_CREATION_STRENGTH_THRESHOLD = (SETUP_ONLY ? 0.0 : 0.5);
static unsigned int LINK_CREATION_COUNT = 10;
static unsigned int LINK_CREATION_MAX_VISIT_ATTEMPTS = LINK_CREATION_COUNT;

static string PRESET_LINKS_FILE_PREFIX = "/opt/das/_PRESET_LINKS_";
static string PRESET_LINKS_FILE = PRESET_LINKS_FILE_PREFIX;
static string CONTEXT_FILE_NAME_PREFIX = "/opt/das/_CONTEXT_DUMP_";
static bool WRITE_CREATED_LINKS_TO_DB = true;
static bool WRITE_CREATED_LINKS_TO_FILE = SETUP_ONLY;
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
static HandleDecoder* DECODER;
static shared_ptr<ServiceBus> bus;
static vector<vector<string>> buffer_determiners;
static map<string, vector<string>> weight_calculation_cache;
static vector<pair<shared_ptr<QueryAnswer>, unsigned int>> recorded_answers;

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

static string hard_wired_metta_expression(const string& handle) {
    STACK_TRACE();
    string answer = "UNDEFINED";
    if (handle == TARGET_CONCEPT_HANDLE) {
        answer = TARGET_CONCEPT;
    } else if (handle == TARGET_PREDICATE_HANDLE) {
        answer = TARGET_PREDICATE;
    } else {
        RAISE_ERROR("Unknown handle: " + handle);
    }
    return answer;
}

static double get_strength(const string& handle) {
    STACK_TRACE();
    auto atom = db->get_atom(handle);
    return atom->custom_attributes.get_or<double>(STRENGTH_TAG, 1.0);
}

static void save_link_metta(shared_ptr<Link> link) {
    STACK_TRACE();
    ofstream file;
    file.open(PRESET_LINKS_FILE, ios::app);
    if (file.is_open()) {
        file << link->custom_attributes.get_or<double>("strength", 1.0) << ","
             << link->metta_representation(*DECODER) << endl;
        file.close();
    } else {
        RAISE_ERROR("Couldn't open file for writing: " + PRESET_LINKS_FILE);
    }
}

static string answer_to_string_2(shared_ptr<QueryAnswer> answer) {
    STACK_TRACE();
    vector<string> paths;
    for (unsigned int i = 0; i < 2; i++) {
        if (answer->get_paths_size() != 2) {
            RAISE_ERROR("Invalid answer: " + answer->to_string());
        }
        string path = "";
        vector<string> path_link = {" -> ", " -> "};
        bool first = true;
        for (string& handle : answer->get_path_vector(i)) {
            auto link = db->get_link(handle);
            auto target1 = db->get_link(link->targets[1]);
            auto target2 = db->get_link(link->targets[2]);
            if (first) {
                first = false;
                path = target1->metta_representation(*DECODER) + path_link[i];
            }
            path += target2->metta_representation(*DECODER);
            path += path_link[i];
        }
        if (answer->get_path_vector(i).size() > 0) {
            path.pop_back();
            path.pop_back();
            path.pop_back();
            path.pop_back();
        }
        paths.push_back(path);
    }
    return "[" + std::to_string(answer->strength) + "]: " + paths[0] + " | " + paths[1];
}

static string answer_to_string_1(shared_ptr<QueryAnswer> answer) {
    STACK_TRACE();
    if (answer->get_paths_size() != 1) {
        RAISE_ERROR("Invalid answer: " + answer->to_string());
    }
    string path = "";
    string path_link = " -> ";
    bool first = true;
    for (string& handle : answer->get_path_vector(0)) {
        auto link = db->get_link(handle);
        auto target1 = db->get_link(link->targets[1]);
        auto target2 = db->get_link(link->targets[2]);
        if (first) {
            first = false;
            path = target1->metta_representation(*DECODER) + path_link;
        }
        path += target2->metta_representation(*DECODER);
        path += path_link;
    }
    if (answer->get_path_vector(0).size() > 0) {
        path.pop_back();
        path.pop_back();
        path.pop_back();
        path.pop_back();
    }
    return "[" + std::to_string(answer->strength) + "]: " + path;
}

static string answer_to_string(shared_ptr<QueryAnswer> answer) {
    STACK_TRACE();
    if (answer->get_paths_size() == 1) {
        return answer_to_string_1(answer);
    } else if (answer->get_paths_size() == 2) {
        return answer_to_string_2(answer);
    } else {
        RAISE_ERROR("Invalid answer: " + answer->to_string());
        return "";
    }
}

static shared_ptr<PatternMatchingQueryProxy> issue_link_building_query(
    const vector<string>& query_tokens, const string& context) {
    STACK_TRACE();
    auto proxy = make_shared<PatternMatchingQueryProxy>(query_tokens, context);
    proxy->parameters[BaseQueryProxy::ATTENTION_CORRELATION] = (unsigned int) BaseQueryProxy::NONE;
    proxy->parameters[BaseQueryProxy::ATTENTION_UPDATE] = (unsigned int) BaseQueryProxy::NONE;
    proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = true;
    proxy->parameters[BaseQueryProxy::USE_LINK_TEMPLATE_CACHE] = false;
    proxy->parameters[BaseQueryProxy::ALLOW_INCOMPLETE_CHAIN_PATH] = false;
    proxy->parameters[PatternMatchingQueryProxy::DISREGARD_IMPORTANCE_FLAG] = false;
    proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = false;
    proxy->parameters[PatternMatchingQueryProxy::MAX_ANSWERS] = (unsigned int) 0;
    proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] = (query_tokens.size() == 1);
    proxy->parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] = USE_MORK;
    proxy->parameters[PatternMatchingQueryProxy::UNIQUE_VALUE_FLAG] = true;

    ServiceBusSingleton::get_instance()->issue_bus_command(proxy);
    return proxy;
}

static shared_ptr<PatternMatchingQueryProxy> issue_weight_count_query(const vector<string>& query_tokens,
                                                                      const string& context) {
    STACK_TRACE();
    auto proxy = make_shared<PatternMatchingQueryProxy>(query_tokens, context);
    proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = true;
    proxy->parameters[BaseQueryProxy::ATTENTION_CORRELATION] = (unsigned int) BaseQueryProxy::NONE;
    proxy->parameters[BaseQueryProxy::ATTENTION_UPDATE] = (unsigned int) BaseQueryProxy::NONE;
    proxy->parameters[BaseQueryProxy::USE_LINK_TEMPLATE_CACHE] = false;
    proxy->parameters[PatternMatchingQueryProxy::DISREGARD_IMPORTANCE_FLAG] = true;
    proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = false;
    proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] = USE_MORK;
    proxy->parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] = false;

    ServiceBusSingleton::get_instance()->issue_bus_command(proxy);
    return proxy;
}

static shared_ptr<PatternMatchingQueryProxy> issue_context_creation_query(
    const vector<string>& query_tokens, const string& context) {
    STACK_TRACE();
    auto proxy = make_shared<PatternMatchingQueryProxy>(query_tokens, context);
    proxy->parameters[BaseQueryProxy::ATTENTION_CORRELATION] = (unsigned int) BaseQueryProxy::NONE;
    proxy->parameters[BaseQueryProxy::ATTENTION_UPDATE] = (unsigned int) BaseQueryProxy::NONE;
    proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = false;
    proxy->parameters[BaseQueryProxy::USE_LINK_TEMPLATE_CACHE] = false;
    proxy->parameters[PatternMatchingQueryProxy::DISREGARD_IMPORTANCE_FLAG] = true;
    proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = false;
    proxy->parameters[PatternMatchingQueryProxy::MAX_ANSWERS] = (unsigned int) 0;
    proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] = (query_tokens.size() == 1);
    proxy->parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] = false;
    proxy->parameters[PatternMatchingQueryProxy::UNIQUE_VALUE_FLAG] = false;

    ServiceBusSingleton::get_instance()->issue_bus_command(proxy);
    return proxy;
}

static void insert_or_update(map<string, double>& count_map, const string& key, double value) {
    STACK_TRACE();
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
    STACK_TRACE();
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
                d = 1;
                for (string& h : query_answer->get_handles_vector()) {
                    d *= get_strength(h);
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

static shared_ptr<Link> add_or_update_link(const string& type_handle,
                                           const string& target1,
                                           const string& target2,
                                           double strength,
                                           const string& context,
                                           bool& link_created_flag) {
    STACK_TRACE();
    link_created_flag = false;
    if (strength < LINK_CREATION_STRENGTH_THRESHOLD) {
        return nullptr;
    }
    LOG_DEBUG("add_or_update_link(" + type_handle + ", " + target1 + ", " + target2 + ", " +
              to_string(strength) + ")");
    shared_ptr<Link> new_link(
        new Link(EXPRESSION, {type_handle, target1, target2}, true, {{STRENGTH_TAG, strength}}));
    LOG_DEBUG("Add or update: " + new_link->to_string());
    string handle = new_link->handle();
    if (db->link_exists(handle)) {
        auto old_link = db->get_atom(handle);
        LOG_DEBUG("Link already exists: " + old_link->to_string());
        if (strength != old_link->custom_attributes.get_or<double>(STRENGTH_TAG, 1)) {
            if (WRITE_CREATED_LINKS_TO_DB) {
                LOG_DEBUG("Updating Link in AtomDB");
                db->delete_link(handle, false);
                db->add_link(new_link.get());
            }
            if (WRITE_CREATED_LINKS_TO_FILE) {
                LOG_DEBUG("Writing Link to file: " + PRESET_LINKS_FILE);
                save_link_metta(new_link);
            }
        }
    } else {
        link_created_flag = true;
        if (WRITE_CREATED_LINKS_TO_DB) {
            LOG_DEBUG("Creating Link in AtomDB");
            if (PRINT_CREATED_LINKS_METTA) {
                LOG_INFO("ADD LINK: [" + std::to_string(strength) + "] " +
                         new_link->metta_representation(*DECODER));
            }
            db->add_link(new_link.get());
            buffer_determiners.push_back({handle, target1, target2});
            AttentionBrokerClient::correlate(set<string>({target1, target2}), context);
        }
        if (WRITE_CREATED_LINKS_TO_FILE) {
            LOG_DEBUG("Writing Link to file: " + PRESET_LINKS_FILE);
            save_link_metta(new_link);
        }
    }
    LOG_LOCAL_DEBUG("Returning from add_or_update_link()");
    return new_link;
}

static void extract_mentioned_predicates(set<string>& mentioned, const string& handle) {
    STACK_TRACE();
    shared_ptr<Node> node;
    shared_ptr<Link> link = db->get_link(handle);
    if (link != nullptr) {
        for (string& target_handle : link->targets) {
            if ((node = db->get_node(target_handle)) != nullptr) {
                if ((node->name != PREDICATE) && (node->name != LOGICAL_AND)) {
                    mentioned.insert(node->name);
                }
            } else {
                extract_mentioned_predicates(mentioned, target_handle);
            }
        }
    }
}

static shared_ptr<Link> add_and_predicate(const string& handle1,
                                          const string& handle2,
                                          const string& context,
                                          bool& link_created_flag) {
    STACK_TRACE();
    if (handle1 == handle2) {
        return nullptr;
    }
    set<string> mentioned_predicates1, mentioned_predicates2;
    extract_mentioned_predicates(mentioned_predicates1, handle1);
    extract_mentioned_predicates(mentioned_predicates2, handle2);
    if (Utils::intersects(mentioned_predicates1, mentioned_predicates2)) {
        LOG_DEBUG("Disregarded AND predicate: " + db->get_atom(handle1)->metta_representation(*DECODER) +
                  " AND " + db->get_atom(handle2)->metta_representation(*DECODER));
        return nullptr;
    }

    string h1, h2;
    if (handle1 < handle2) {
        h1 = handle1;
        h2 = handle2;
    } else {
        h1 = handle2;
        h2 = handle1;
    }

    return add_or_update_link(LOGICAL_AND_HANDLE, h1, h2, 1.0, context, link_created_flag);
}

static bool build_and_predicate_link(shared_ptr<QueryAnswer> query_answer,
                                     const string& context,
                                     const string& custom_handle) {
    STACK_TRACE();
    string predicate1 = query_answer->get(PREDICATE1);
    string predicate2 = (custom_handle == "" ? query_answer->get(PREDICATE2) : custom_handle);
    string concept1 = query_answer->get(CONCEPT);

    if (predicate1 == predicate2) {
        LOG_DEBUG("Skipping link building because targets are the same: " + predicate1);
        return false;
    }

    bool link_created_flag;
    shared_ptr<Link> new_predicate =
        add_and_predicate(predicate1, predicate2, context, link_created_flag);
    if ((link_created_flag) && (new_predicate != nullptr)) {
        double strength = 1;
        for (string& h : query_answer->get_handles_vector()) {
            strength *= get_strength(h);
        }
        add_or_update_link(
            EVALUATION_HANDLE, new_predicate->handle(), concept1, strength, context, link_created_flag);
        return link_created_flag;
    } else {
        return false;
    }
}

static bool build_implication_link(shared_ptr<QueryAnswer> query_answer,
                                   const string& context,
                                   const string& custom_handle,
                                   set<pair<string, string>>& visited,
                                   bool& none_visited) {
    STACK_TRACE();
    none_visited = true;
    string predicates[2];
    string metta_predicates[2];
    if (custom_handle == "") {
        predicates[0] = query_answer->get(PREDICATE1);
        predicates[1] = query_answer->get(PREDICATE2);
        metta_predicates[0] = query_answer->metta_expression[predicates[0]];
        metta_predicates[1] = query_answer->metta_expression[predicates[1]];
    } else {
        predicates[0] = query_answer->get(PREDICATE1);
        predicates[1] = custom_handle;
        metta_predicates[0] = query_answer->metta_expression[predicates[0]];
        metta_predicates[1] = hard_wired_metta_expression(custom_handle);
    }

    if (predicates[0] == predicates[1]) {
        LOG_DEBUG("Skipping link building because targets are the same: " + predicates[0]);
        return false;
    }

    if (visited.find({predicates[0], predicates[1]}) != visited.end()) {
        LOG_DEBUG("Skipping link building because targets have already been visited this cycle: " +
                  predicates[0] + " " + predicates[1]);
        return false;
    }

    set<string> mentioned_predicates1, mentioned_predicates2;
    extract_mentioned_predicates(mentioned_predicates1, predicates[0]);
    extract_mentioned_predicates(mentioned_predicates2, predicates[1]);
    if (Utils::intersects(mentioned_predicates1, mentioned_predicates2)) {
        LOG_DEBUG("Disregarded IMPLICATION predicates: " +
                  db->get_atom(predicates[0])->metta_representation(*DECODER) + " <=> " +
                  db->get_atom(predicates[1])->metta_representation(*DECODER));
        return false;
    }

    LOG_DEBUG("Visiting: " + predicates[0] + " " + predicates[1]);
    none_visited = false;
    visited.insert({predicates[0], predicates[1]});

    // build Evaluation of the AND of both predicates
    build_and_predicate_link(query_answer, context, custom_handle);

    vector<vector<string>> query;
    for (unsigned int i = 0; i < 2; i++) {
        // clang-format off
        if (USE_MORK) {
            /*
            vector<string> metta_query = {metta_expr3(EVALUATION, metta_predicates[i], metta_var(CONCEPT1))};
            */
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
    bool link_created_flag = false;
    if (count_intersection > 0) {
        if (count_0 > 0) {
            if (count_1 > 0) {
                if (count_0 < count_1) {
                    add_or_update_link(IMPLICATION_HANDLE,
                                       predicates[0],
                                       predicates[1],
                                       count_intersection / count_0,
                                       context,
                                       link_created_flag);
                } else {
                    add_or_update_link(IMPLICATION_HANDLE,
                                       predicates[1],
                                       predicates[0],
                                       count_intersection / count_1,
                                       context,
                                       link_created_flag);
                }
            } else {
                add_or_update_link(IMPLICATION_HANDLE,
                                   predicates[0],
                                   predicates[1],
                                   count_intersection / count_0,
                                   context,
                                   link_created_flag);
            }
        } else {
            if (count_1 > 0) {
                add_or_update_link(IMPLICATION_HANDLE,
                                   predicates[1],
                                   predicates[0],
                                   count_intersection / count_1,
                                   context,
                                   link_created_flag);
            }
        }
    }
    return link_created_flag;
}

static bool build_equivalence_link(shared_ptr<QueryAnswer> query_answer,
                                   const string& context,
                                   const string& custom_handle,
                                   set<pair<string, string>>& visited,
                                   bool& none_visited) {
    STACK_TRACE();
    none_visited = true;
    string concepts[2];
    string metta_concepts[2];
    if (custom_handle == "") {
        concepts[0] = query_answer->get(CONCEPT1);
        concepts[1] = query_answer->get(CONCEPT2);
        metta_concepts[0] = query_answer->metta_expression[concepts[0]];
        metta_concepts[1] = query_answer->metta_expression[concepts[1]];
    } else {
        concepts[0] = query_answer->get(CONCEPT1);
        concepts[1] = custom_handle;
        metta_concepts[0] = query_answer->metta_expression[concepts[0]];
        metta_concepts[1] = hard_wired_metta_expression(custom_handle);
    }

    if (concepts[0] == concepts[1]) {
        LOG_DEBUG("Skipping link building because targets are the same: " + concepts[0]);
        return false;
    }
    pair<string, string> p;
    if (concepts[0] < concepts[1]) {
        p = {concepts[0], concepts[1]};
    } else {
        p = {concepts[1], concepts[0]};
    }
    if (visited.find(p) != visited.end()) {
        LOG_DEBUG("Skipping link building because targets have already been visited this cycle: " +
                  concepts[0] + " " + concepts[1]);
        return false;
    }

    LOG_DEBUG("Visiting: " + concepts[0] + " " + concepts[1]);
    none_visited = false;
    visited.insert(p);

    vector<vector<string>> query;
    for (unsigned int i = 0; i < 2; i++) {
        // clang-format off
        if (USE_MORK) {
            /*
            vector<string> metta_query = {metta_expr3(EVALUATION, metta_var(PREDICATE1), metta_concepts[i])};
            */
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
    bool link_created_flag1 = false;
    bool link_created_flag2 = false;
    if ((count_intersection > 0) && (count_union > 0)) {
        double strength = count_intersection / count_union;
        add_or_update_link(
            EQUIVALENCE_HANDLE, concepts[0], concepts[1], strength, context, link_created_flag1);
        add_or_update_link(
            EQUIVALENCE_HANDLE, concepts[1], concepts[0], strength, context, link_created_flag2);
    }
    return (link_created_flag1 || link_created_flag2);
}

static bool build_evaluation_link(shared_ptr<QueryAnswer> query_answer,
                                  const string& context,
                                  const string& custom_handle,
                                  set<pair<string, string>>& visited,
                                  bool& none_visited) {
    STACK_TRACE();
    none_visited = true;
    string predicate = query_answer->get(PREDICATE);
    string concept_ = query_answer->get(CONCEPT);
    if (visited.find({predicate, concept_}) != visited.end()) {
        LOG_DEBUG("Skipping link building because targets have already been visited this cycle: " +
                  predicate + " " + concept_);
        return false;
    }
    LOG_DEBUG("Visiting: " + predicate + " " + concept_);
    none_visited = false;
    visited.insert({predicate, concept_});
    double strength = 1;
    for (string& h : query_answer->get_handles_vector()) {
        strength *= get_strength(h);
    }
    bool link_created_flag;
    add_or_update_link(EVALUATION_HANDLE, predicate, concept_, strength, context, link_created_flag);
    return link_created_flag;
}

static void build_links(const vector<string>& query,
                        const string& context,
                        const string& custom_handle,
                        bool (*build_link)(shared_ptr<QueryAnswer> query_answer,
                                           const string& context,
                                           const string& custom_handle,
                                           set<pair<string, string>>& visited,
                                           bool& none_visited)) {
    STACK_TRACE();
    auto proxy = issue_link_building_query(query, context);
    unsigned int count_created = 0;
    unsigned int count_attempts = 0;
    shared_ptr<QueryAnswer> query_answer;
    set<pair<string, string>> visited;
    while (true) {
        if ((query_answer = proxy->pop()) == nullptr) {
            if (proxy->finished()) {
                break;
            }
            Utils::sleep();
        } else {
            LOG_DEBUG("Processing query answer " + to_string(count_created) + ": " +
                      query_answer->to_string(USE_MORK));
            bool none_visited = true;
            if (build_link(query_answer, context, custom_handle, visited, none_visited)) {
                count_created++;
                count_attempts = 0;
                if (count_created == LINK_CREATION_COUNT) {
                    break;
                }
            } else if (!none_visited) {
                count_attempts++;
                if (count_attempts == LINK_CREATION_MAX_VISIT_ATTEMPTS) {
                    break;
                }
            }
        }
    }
    if (!proxy->finished()) {
        proxy->abort();
    }
    LOG_DEBUG("Built " + to_string(count_created) + " links");
}

// clang-format off
static void query_evolution(
    const vector<string>& query_to_evolve,
    const vector<vector<string>>& correlation_query_template,
    unsigned int iteration,
    const string& context) {

    STACK_TRACE();
    QueryAnswerElement qa_predicate(PREDICATE);
    QueryAnswerElement qa_concept(CONCEPT);
    QueryAnswerElement qa_path(QueryAnswerElement::ALL_PATH_HANDLES);
    QueryAnswerElement qa_nothing;
    QueryAnswerElement qa_everything(QueryAnswerElement::EVERYTHING);

    vector<map<string, QueryAnswerElement>> correlation_query_constants = {
        {{V1, qa_predicate}},
        {{V2, qa_concept}}
    };
    vector<vector<pair<QueryAnswerElement, QueryAnswerElement>>> correlation_mapping_1 = {
        {{qa_concept, qa_concept}},
        {{qa_predicate, qa_predicate}}
    };
    vector<vector<pair<QueryAnswerElement, QueryAnswerElement>>> correlation_mapping = {
        {{qa_concept, qa_concept}, {qa_path, qa_nothing}},
        {{qa_predicate, qa_predicate}, {qa_path, qa_nothing}}
    };

    QueryEvolutionProxy* proxy_ptr = new QueryEvolutionProxy(
        query_to_evolve,
        correlation_query_template,
        correlation_query_constants,
        correlation_mapping,
        context,
        FITNESS_FUNCTION);

    shared_ptr<QueryEvolutionProxy> proxy(proxy_ptr);

    proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = false;
    proxy->parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] = true;
    proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] = USE_MORK;
    proxy->parameters[BaseQueryProxy::ALLOW_INCOMPLETE_CHAIN_PATH] = true;
    proxy->parameters[BaseQueryProxy::MAX_BUNDLE_SIZE] = (unsigned int) 1000;
    proxy->parameters[PatternMatchingQueryProxy::DISREGARD_IMPORTANCE_FLAG] = false;
    proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = false;
    proxy->parameters[PatternMatchingQueryProxy::UNIQUE_VALUE_FLAG] = false;
    proxy->parameters[PatternMatchingQueryProxy::COUNT_FLAG] = false;
    proxy->parameters[QueryEvolutionProxy::POPULATION_SIZE] = (unsigned int) POPULATION_SIZE;
    proxy->parameters[QueryEvolutionProxy::MAX_GENERATIONS] = (unsigned int) MAX_GENERATIONS;
    proxy->parameters[QueryEvolutionProxy::ELITISM_RATE] = (double) ELITISM_RATE;
    proxy->parameters[QueryEvolutionProxy::SELECTION_RATE] = (double) SELECTION_RATE;

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
                LOG_INFO("ANSWER: " + answer_to_string(query_answer));
                recorded_answers.push_back({query_answer, iteration});
            }
        }
    }
    LOG_INFO("Total answers in iteration " << count_iterations++ << ": " << count_answers);
}
// clang-format on

static void add_to_context_file(const filesystem::path& context_file_name,
                                const string& context,
                                ContextTaskType task,
                                const vector<string>& query,
                                const vector<vector<QueryAnswerElement>>& selector) {
    STACK_TRACE();
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
                switch (task) {
                    case DETERMINER:
                        mnemonic = "DET";
                    case CORRELATION:
                        if (mnemonic == "") mnemonic = "COR";
                        for (auto pair : selector) {
                            if (pair.size() != 2) {
                                RAISE_ERROR("Invalid context task selector for " + mnemonic);
                                return;
                            }
                            file << mnemonic << " " << query_answer->get(pair[0]) << " "
                                 << query_answer->get(pair[1]) << endl;
                        }
                        break;
                    case ACTIVATION:
                        mnemonic = "ACT";
                        if (selector.size() != 1) {
                            RAISE_ERROR("Invalid context task selector for " + mnemonic);
                        }
                        file << mnemonic;
                        for (auto element : selector[0]) {
                            file << " " << query_answer->get(element);
                        }
                        file << endl;
                        break;
                    default:
                        RAISE_ERROR("Invalid context creation task: " +
                                    std::to_string((unsigned int) task));
                }
            }
        }
        file.close();
    } else {
        RAISE_ERROR("Couldn't open file for writing: " + string(context_file_name));
    }
}

static void add_to_context_file(const filesystem::path& context_file_name,
                                const string& context,
                                ContextTaskType task,
                                const vector<string>& handles) {
    STACK_TRACE();
    ofstream file;
    file.open(context_file_name, ios::app);
    if (file.is_open()) {
        for (string handle : handles) {
            switch (task) {
                case DETERMINER:
                    RAISE_ERROR("Not implemented");
                    break;
                case CORRELATION:
                    RAISE_ERROR("Not implemented");
                    break;
                case ACTIVATION:
                    file << "ACT " << handle << endl;
                    break;
                default:
                    RAISE_ERROR("Invalid context creation task: " + std::to_string((unsigned int) task));
            }
        }
        file.close();
    } else {
        RAISE_ERROR("Couldn't open file for writing: " + string(context_file_name));
    }
}

static void add_preset_links(const vector<string>& implication_to_target_predicate_metta_query,
                             const vector<string>& implication_to_target_predicate_query,
                             const vector<string>& equivalence_to_target_concept_metta_query,
                             const vector<string>& equivalence_to_target_concept_query,
                             const string& context) {
    STACK_TRACE();
    ifstream file(PRESET_LINKS_FILE);
    if (file.is_open()) {
        LOG_INFO("Reading preset links from file: " + PRESET_LINKS_FILE);
        vector<string> line;
        unsigned int count = 0;
        while (Utils::read_and_split(line, file, ',')) {
            shared_ptr<atoms::MettaParserActions> parser_handler =
                make_shared<atoms::MettaParserActions>();
            MettaParser parser(line[1], parser_handler);
            parser.parse();
            auto link = std::dynamic_pointer_cast<Link>(parser_handler->element_stack.top());
            link->custom_attributes["strength"] = (double) Utils::string_to_float(line[0]);
            LOG_DEBUG("Adding Link: [" + line[0] + "] " + line[1]);
            db->add_link(link.get(), false);
            count++;
            line.clear();
            buffer_determiners.push_back({link->handle(), link->targets[1], link->targets[2]});
            AttentionBrokerClient::correlate({link->targets[1], link->targets[2]}, context);
        }
        LOG_INFO(std::to_string(count) + " preset links read.");
    } else {
        LOG_INFO("Couldn't open file: " + PRESET_LINKS_FILE);
        LOG_INFO("Building Implication links to TARGET_PREDICATE");
        build_links((USE_MORK ? implication_to_target_predicate_metta_query
                              : implication_to_target_predicate_query),
                    context,
                    TARGET_PREDICATE_HANDLE,
                    build_implication_link);
        LOG_INFO("Building Equivalence links to TARGET_CONCEPT");
        build_links(
            (USE_MORK ? equivalence_to_target_concept_metta_query : equivalence_to_target_concept_query),
            context,
            TARGET_CONCEPT_HANDLE,
            build_equivalence_link);
    }
    file.close();
    LOG_INFO("Updating determiners in AttentionBroker");
    AttentionBrokerClient::set_determiners(buffer_determiners, context);
    buffer_determiners.clear();
}

static void run(const string& context_tag) {
    STACK_TRACE();
    // clang-format off
    vector<string> implication_query = {
        ANDNOT_OPERATOR, "3",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE1,
                VARIABLE, CONCEPT,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE2,
                VARIABLE, CONCEPT,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, IMPLICATION,
                VARIABLE, PREDICATE1,
                VARIABLE, PREDICATE2,
    };

    vector<string> implication_to_target_predicate_query = {
        AND_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE1,
                VARIABLE, CONCEPT,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                ATOM, TARGET_PREDICATE_HANDLE,
                VARIABLE, CONCEPT,
    };
    vector<string> implication_to_target_predicate_metta_query = {
        metta_and(
            metta_expr3(EVALUATION, metta_var(PREDICATE1), metta_var(CONCEPT)),
            metta_expr3(EVALUATION, TARGET_PREDICATE, metta_var(CONCEPT)))
    };

    vector<string> equivalence_query = {
        ANDNOT_OPERATOR, "3",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE,
                VARIABLE, CONCEPT1,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE,
                VARIABLE, CONCEPT2,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EQUIVALENCE,
                VARIABLE, CONCEPT1,
                VARIABLE, CONCEPT2,
    };

    vector<string> equivalence_to_target_concept_query = {
        AND_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE,
                VARIABLE, CONCEPT1,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE,
                ATOM, TARGET_CONCEPT_HANDLE,
    };
    vector<string> equivalence_to_target_concept_metta_query = {
        metta_and(
            metta_expr3(EVALUATION, metta_var(PREDICATE), metta_var(CONCEPT1)),
            metta_expr3(EVALUATION, metta_var(PREDICATE), TARGET_CONCEPT))
    };

    vector<string> evaluation_fixed_predicate_query = {
        ANDNOT_OPERATOR, "3",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE,
                VARIABLE, CONCEPT1,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EQUIVALENCE,
                VARIABLE, CONCEPT1,
                VARIABLE, CONCEPT,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE,
                VARIABLE, CONCEPT,
    };

    vector<string> evaluation_fixed_concept_query = {
        ANDNOT_OPERATOR, "3",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE1,
                VARIABLE, CONCEPT,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, IMPLICATION,
                VARIABLE, PREDICATE1,
                VARIABLE, PREDICATE,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE,
                VARIABLE, CONCEPT,
    };

    vector<string> implication_to_path_query = {
        ANDNOT_OPERATOR, "4",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE1,
                VARIABLE, CONCEPT,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE2,
                VARIABLE, CONCEPT,
            CHAIN_OPERATOR, "0", "1", "2",
                VARIABLE, PREDICATE2,
                ATOM, TARGET_PREDICATE_HANDLE,
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, IMPLICATION,
                    VARIABLE, V1,
                    VARIABLE, V2,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, IMPLICATION,
                VARIABLE, PREDICATE1,
                VARIABLE, PREDICATE2,
    };

    vector<string> equivalence_to_path_query = {
        ANDNOT_OPERATOR, "4",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE,
                VARIABLE, CONCEPT1,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION,
                VARIABLE, PREDICATE,
                VARIABLE, CONCEPT2,
            CHAIN_OPERATOR, "0", "1", "2",
                VARIABLE, CONCEPT2,
                ATOM, TARGET_CONCEPT_HANDLE,
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, EQUIVALENCE,
                    VARIABLE, V1,
                    VARIABLE, V2,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EQUIVALENCE,
                VARIABLE, CONCEPT1,
                VARIABLE, CONCEPT2,
    };

    vector<string> query_to_evolve = {
        OR_OPERATOR, "3",
            AND_OPERATOR, "2",
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, EVALUATION,
                    VARIABLE, PREDICATE,
                    ATOM, TARGET_CONCEPT_HANDLE,
                CHAIN_OPERATOR, "0", "1", "2",
                    VARIABLE, PREDICATE,
                    ATOM, TARGET_PREDICATE_HANDLE,
                    LINK_TEMPLATE, EXPRESSION, "3",
                        NODE, SYMBOL, IMPLICATION,
                        VARIABLE, PREDICATE1,
                        VARIABLE, PREDICATE2,
            AND_OPERATOR, "2",
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, EVALUATION,
                    ATOM, TARGET_PREDICATE_HANDLE,
                    VARIABLE, CONCEPT,
                CHAIN_OPERATOR, "0", "1", "2",
                    VARIABLE, CONCEPT,
                    ATOM, TARGET_CONCEPT_HANDLE,
                    LINK_TEMPLATE, EXPRESSION, "3",
                        NODE, SYMBOL, EQUIVALENCE,
                        VARIABLE, CONCEPT1,
                        VARIABLE, CONCEPT2,
            AND_OPERATOR, "3",
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, EVALUATION,
                    VARIABLE, PREDICATE,
                    VARIABLE, CONCEPT,
                CHAIN_OPERATOR, "0", "1", "2",
                    VARIABLE, PREDICATE,
                    ATOM, TARGET_PREDICATE_HANDLE,
                    LINK_TEMPLATE, EXPRESSION, "3",
                        NODE, SYMBOL, IMPLICATION,
                        VARIABLE, PREDICATE1,
                        VARIABLE, PREDICATE2,
                CHAIN_OPERATOR, "0", "1", "2",
                    VARIABLE, CONCEPT,
                    ATOM, TARGET_CONCEPT_HANDLE,
                    LINK_TEMPLATE, EXPRESSION, "3",
                        NODE, SYMBOL, EQUIVALENCE,
                        VARIABLE, CONCEPT1,
                        VARIABLE, CONCEPT2,
    };
    vector<string> metta_query_to_evolve = {
        metta_or(
            metta_and(
                metta_expr3(EVALUATION, metta_var(PREDICATE), TARGET_CONCEPT),
                metta_chain(metta_var(PREDICATE), TARGET_PREDICATE, metta_expr3(IMPLICATION, metta_var(PREDICATE1) , metta_var(PREDICATE2)))
            ),
            metta_or(
                metta_and(
                    metta_expr3(EVALUATION, TARGET_PREDICATE, metta_var(CONCEPT)),
                    metta_chain(metta_var(CONCEPT), TARGET_CONCEPT, metta_expr3(EQUIVALENCE, metta_var(CONCEPT1) , metta_var(CONCEPT2)))
                ),
                metta_and(
                    metta_and(
                        metta_expr3(EVALUATION, metta_var(PREDICATE), metta_var(CONCEPT)),
                        metta_chain(metta_var(PREDICATE), TARGET_PREDICATE, metta_expr3(IMPLICATION, metta_var(V1) , metta_var(V2)))
                    ),
                    metta_and(
                        metta_expr3(EVALUATION, metta_var(PREDICATE), metta_var(CONCEPT)),
                        metta_chain(metta_var(CONCEPT), TARGET_CONCEPT, metta_expr3(EQUIVALENCE, metta_var(V1) , metta_var(V2)))
                    )
                )
            )
        )
    };

    vector<vector<string>> correlation_query_template = {
        {LINK_TEMPLATE, EXPRESSION, "3",
            NODE, SYMBOL, EVALUATION,
            VARIABLE, V1,
            VARIABLE, CONCEPT},
        {LINK_TEMPLATE, EXPRESSION, "3",
            NODE, SYMBOL, EVALUATION,
            VARIABLE, PREDICATE,
            VARIABLE, V2}
    };
    vector<vector<string>> correlation_metta_query_template = {
        {metta_expr3(EVALUATION, metta_var(V1), metta_var(CONCEPT))},
        {metta_expr3(EVALUATION, metta_var(PREDICATE), metta_var(V2))}
    };

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
            ATOM, TARGET_PREDICATE_HANDLE,
            VARIABLE, CONCEPT,
    };
    vector<string> context_activation_metta_query1 = {
        metta_expr3(EVALUATION, TARGET_PREDICATE, metta_var(CONCEPT))
    };

    vector<string> context_activation_query2 = {
        LINK_TEMPLATE, EXPRESSION, "3",
            NODE, SYMBOL, EVALUATION,
            VARIABLE, PREDICATE,
            ATOM, TARGET_CONCEPT_HANDLE,
    };
    vector<string> context_activation_metta_query2 = {
        metta_expr3(EVALUATION, metta_var(PREDICATE), TARGET_CONCEPT)
    };
    // clang-format on

    if (SETUP_ONLY) {
        LOG_INFO("****************************** Running for SETUP only ******************************");
    }
    LOG_INFO("Setting up context for tag: " + context_tag);
    string context = Hasher::context_handle(context_tag);
    PRESET_LINKS_FILE = PRESET_LINKS_FILE_PREFIX + context + ".txt";
    filesystem::path context_file_name = CONTEXT_FILE_NAME_PREFIX + context + ".txt";
    if (!filesystem::exists(context_file_name)) {
        LOG_INFO("Context file doesn't exist. Creating it...");
        QueryAnswerElement qe_predicate(PREDICATE);
        QueryAnswerElement qe_concept(CONCEPT);
        QueryAnswerElement qe_toplevel(0);
        LOG_INFO("Creating determiners");
        add_to_context_file(context_file_name,
                            context,
                            DETERMINER,
                            context_determiner_query,
                            {{qe_toplevel, qe_predicate}, {qe_toplevel, qe_concept}});
        LOG_INFO("Making correlations");
        add_to_context_file(context_file_name,
                            context,
                            CORRELATION,
                            context_correlation_query,
                            {{qe_concept, qe_predicate}, {qe_predicate, qe_concept}});
        LOG_INFO("Spreading activation");
        add_to_context_file(context_file_name,
                            context,
                            ACTIVATION,
                            (USE_MORK ? context_activation_metta_query1 : context_activation_query1),
                            {{qe_concept}});
        add_to_context_file(context_file_name,
                            context,
                            ACTIVATION,
                            (USE_MORK ? context_activation_metta_query2 : context_activation_query2),
                            {{qe_predicate}});
        add_to_context_file(
            context_file_name, context, ACTIVATION, {TARGET_PREDICATE_HANDLE, TARGET_CONCEPT_HANDLE});

    } else {
        LOG_INFO("Context file already exists. Reusing it...");
    }
    LOG_INFO("Updating AttentionBroker");
    AttentionBrokerClient::drop_and_load_context(context, string(context_file_name));
    LOG_INFO("Context " + context + " is ready");

    add_preset_links(implication_to_target_predicate_metta_query,
                     implication_to_target_predicate_query,
                     equivalence_to_target_concept_metta_query,
                     equivalence_to_target_concept_query,
                     context);

    if (SETUP_ONLY) {
        LOG_INFO("******************************     SETUP finished     ******************************");
        return;
    }

    for (unsigned int iteration = 1; iteration <= NUM_ITERATIONS; iteration++) {
        LOG_INFO("--------------------------------------------------------------------------------");
        LOG_INFO("Iteration " + to_string(iteration));
        LOG_INFO("--------------------------------------------------------------------------------");
        LOG_INFO("----- Building links");
        LOG_INFO("Building Implication links");
        build_links(implication_query, context, "", build_implication_link);
        // LOG_INFO("Building Implication links to paths");
        // build_links(implication_to_path_query, context, "", build_implication_link);
        LOG_INFO("Building Equivalence links");
        build_links(equivalence_query, context, "", build_equivalence_link);
        // LOG_INFO("Building Equivalence links to paths");
        // build_links(equivalence_to_path_query, context, "", build_equivalence_link);
        LOG_INFO("Building Evaluation links");
        build_links(evaluation_fixed_predicate_query, context, "", build_evaluation_link);
        build_links(evaluation_fixed_concept_query, context, "", build_evaluation_link);
        LOG_INFO("----- Updating AttentionBroker");
        AttentionBrokerClient::set_determiners(buffer_determiners, context);
        buffer_determiners.clear();
        LOG_INFO("----- Evolving query");
        query_evolution((USE_MORK ? metta_query_to_evolve : query_to_evolve),
                        (USE_MORK ? correlation_metta_query_template : correlation_query_template),
                        iteration,
                        context);
    }

    LOG_INFO("--------------------------------------------------------------------------------");
    LOG_INFO("Finished. Recorded results:");
    for (auto pair : recorded_answers) {
        LOG_INFO("ANSWER: " + answer_to_string(pair.first) + " [" + std::to_string(pair.second) + "]");
    }
    LOG_INFO("--------------------------------------------------------------------------------");
}

static void insert_type_symbols() {
    STACK_TRACE();
    vector<string> to_insert = {EQUIVALENCE, IMPLICATION, LOGICAL_AND};
    Node* node;
    for (string node_name : to_insert) {
        node = new Node(SYMBOL, node_name);
        db->add_node(node, false);
        delete (node);
    }
}

int main(int argc, char* argv[]) {
    STACK_TRACE();
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
    TARGET_PREDICATE = argv[++cursor];
    TARGET_CONCEPT = argv[++cursor];

    RENT_RATE = Utils::string_to_float(string(argv[++cursor]));
    SPREADING_RATE_LOWERBOUND = Utils::string_to_float(string(argv[++cursor]));
    SPREADING_RATE_UPPERBOUND = Utils::string_to_float(string(argv[++cursor]));

    ELITISM_RATE = (double) Utils::string_to_float(string(argv[++cursor]));
    SELECTION_RATE = (double) Utils::string_to_float(string(argv[++cursor]));
    POPULATION_SIZE = (unsigned int) Utils::string_to_int(string(argv[++cursor]));
    MAX_GENERATIONS = (unsigned int) Utils::string_to_int(string(argv[++cursor]));
    NUM_ITERATIONS = (unsigned int) Utils::string_to_int(string(argv[++cursor]));

    if (cursor != 15) {
        RAISE_ERROR("Error setting up parameters");
    }

    auto json_config = JsonConfigParser::load(config_file);
    auto atomdb_config = json_config.at_path("atomdb").get_or<JsonConfig>(JsonConfig());
    SystemParametersSingleton::init(json_config);
    AtomDBSingleton::init(atomdb_config);

    db = AtomDBSingleton::get_instance();
    DECODER = static_pointer_cast<HandleDecoder>(db).get();
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
    MettaParser predicate_p(TARGET_PREDICATE, predicate_pa);
    MettaParser concept_p(TARGET_CONCEPT, concept_pa);
    predicate_p.parse();
    concept_p.parse();
    TARGET_PREDICATE_HANDLE = predicate_pa->metta_expression_handle,
    TARGET_CONCEPT_HANDLE = concept_pa->metta_expression_handle,

    LOG_INFO("Target predicate: " + TARGET_PREDICATE + " Handle: " + TARGET_PREDICATE_HANDLE);
    LOG_INFO("Target concept: " + TARGET_CONCEPT + " Handle: " + TARGET_CONCEPT_HANDLE);

    run(context_tag);

    double best_strength = 0;
    unsigned int iteration = 0;
    if (recorded_answers.size() > 0) {
        best_strength = recorded_answers.back().first->strength;
        iteration = recorded_answers.back().second;
    }
    vector<string> test_label = {context_tag,
                                 to_string(RENT_RATE),
                                 to_string(SPREADING_RATE_LOWERBOUND),
                                 to_string(SPREADING_RATE_UPPERBOUND),
                                 to_string(ELITISM_RATE),
                                 to_string(SELECTION_RATE)};
    LOG_INFO("FINAL_RESULT " + to_string(best_strength) + " " + to_string(iteration) + " " +
             Utils::join(test_label, '_') + " " + answer_to_string(recorded_answers.back().first));

    return 0;
}
