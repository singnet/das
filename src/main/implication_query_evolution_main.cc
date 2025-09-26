#include <signal.h>

#define LOG_LEVEL INFO_LEVEL

#include <iomanip>
#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "AtomSpace.h"
#include "AttentionBrokerClient.h"
#include "Context.h"
#include "CountLetterFunction.h"
#include "FitnessFunctionRegistry.h"
#include "QueryAnswer.h"
#include "QueryEvolutionProxy.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"
#include "Hasher.h"
#include "Logger.h"

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
#define EVALUATION "EVALUATION"
#define PREDICATE "PREDICATE"
#define CONCEPT "CONCEPT"
#define EQUIVALENCE "EQUIVALENCE"
#define IMPLICATION "IMPLICATION"

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

// Misc
#define STRENGTH "strength"
#define FITNESS_FUNCTION "multiply_strength"

static string IMPLICATION_HANDLE = Hasher::node_handle(SYMBOL, IMPLICATION);
static string EQUIVALENCE_HANDLE = Hasher::node_handle(SYMBOL, EQUIVALENCE);
static float RENT_RATE = 0.25;
static float SPREADING_RATE_LOWERBOUND = 0.90;
static float SPREADING_RATE_UPPERBOUND = 0.90;
static double SELECTION_RATE = 0.10;
static double ELITISM_RATE = 0.08;
static unsigned int LINK_BUILDING_QUERY_SIZE = 200;
static unsigned int POPULATION_SIZE = 50;
static unsigned int MAX_GENERATIONS = 20;
static unsigned int NUM_ITERATIONS = 10;

using namespace std;
using namespace atomdb;
using namespace atom_space;
using namespace query_engine;
using namespace evolution;
using namespace service_bus;
using namespace attention_broker;

static shared_ptr<AtomDB> db;
static shared_ptr<ServiceBus> bus;
static vector<vector<string>> buffer_determiners;

static string get_predicate_name(string handle) {
    shared_ptr<Link> predicate_link = dynamic_pointer_cast<Link>(db->get_atom(handle));
    shared_ptr<Node> predicate_symbol =
        dynamic_pointer_cast<Node>(db->get_atom(predicate_link->targets[1]));
    return predicate_symbol->name;
}

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
        if (source_predicate != last_predicate) {
            Utils::error("Bad predicate chaining: " + source_predicate + " != " + last_predicate);
        }
        target_predicate = get_predicate_name(implication_link->targets[2]);
        cout << " --> " << target_predicate;
        last_predicate = target_predicate;
    }
    cout << endl;
}

static shared_ptr<PatternMatchingQueryProxy> issue_query(const vector<string>& query_tokens,
                                                         const string& context,
                                                         unsigned int max_answers) {
    auto proxy = make_shared<PatternMatchingQueryProxy>(query_tokens, context);
    proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = true;
    proxy->parameters[BaseQueryProxy::ATTENTION_UPDATE_FLAG] = false;
    proxy->parameters[BaseQueryProxy::USE_LINK_TEMPLATE_CACHE] = false;
    proxy->parameters[PatternMatchingQueryProxy::MAX_ANSWERS] = (unsigned int) max_answers;
    proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = false;
    proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] = false;
    proxy->parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] = false;

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
                           double& count_0,
                           double& count_1,
                           double& count_intersection,
                           double& count_union) {
    shared_ptr<PatternMatchingQueryProxy> proxy[2];

    for (unsigned int i = 0; i < 2; i++) {
        proxy[i] = make_shared<PatternMatchingQueryProxy>(query_tokens[i], context);
        proxy[i]->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = false;
        proxy[i]->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = true;
        proxy[i]->parameters[BaseQueryProxy::ATTENTION_UPDATE_FLAG] = false;
        proxy[i]->parameters[BaseQueryProxy::USE_LINK_TEMPLATE_CACHE] = false;
        proxy[i]->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] = false;
        proxy[i]->parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] = false;
    }

    count_0 = 0.0;
    count_1 = 0.0;
    count_intersection = 0.0;
    count_union = 0.0;

    map<string, double> count_map[2];
    map<string, double> count_map_union;
    map<string, double> count_map_intersection;

    double d;
    string handle;
    shared_ptr<QueryAnswer> query_answer;
    for (unsigned int i = 0; i < 2; i++) {
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
    count_map_union = count_map[0];
    for (auto pair : count_map[1]) {
        auto iterator = count_map[0].find(pair.first);
        if (iterator == count_map[0].end()) {
            insert_or_update(count_map_union, pair.first, pair.second);
        } else {
            insert_or_update(count_map_union, pair.first, pair.second);
            insert_or_update(count_map_intersection, pair.first, pair.second);
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
}

static void add_or_update_link(const string& type_handle,
                               const string& target1,
                               const string& target2,
                               double strength) {
    Link new_link(EXPRESSION, {type_handle, target1, target2}, true, {{STRENGTH, strength}});
    string handle = new_link.handle();
    if (db->link_exists(handle)) {
        auto old_link = db->get_atom(handle);
        if (strength > old_link->custom_attributes.get<double>(STRENGTH)) {
            db->delete_link(handle, false);
            db->add_link(&new_link);
        }
    } else {
        db->add_link(&new_link);
        buffer_determiners.push_back({handle, target1, target2});
    }
}

static void build_implication_link(shared_ptr<QueryAnswer> query_answer, const string& context) {
    if (query_answer->handles[0] == query_answer->handles[1]) {
        return;
    }

    string predicate[2] = {query_answer->handles[0], query_answer->handles[1]};
    vector<vector<string>> query;

    for (unsigned int i = 0; i < 2; i++) {
        // clang-format off
        query[i] = {
            OR_OPERATOR, "2"
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
        };
        // clang-format on
    }
    double count_0, count_1, count_intersection, count_union;
    QueryAnswerElement target_element(CONCEPT1);
    compute_counts(query, context, target_element, count_0, count_1, count_intersection, count_union);
    if (count_0 > 0) {
        double strength = count_intersection / count_0;
        add_or_update_link(IMPLICATION_HANDLE, predicate[0], predicate[1], strength);
    }
    if (count_1 > 0) {
        double strength = count_intersection / count_1;
        add_or_update_link(IMPLICATION_HANDLE, predicate[1], predicate[0], strength);
    }
}

static void build_equivalence_link(shared_ptr<QueryAnswer> query_answer, const string& context) {
    if (query_answer->handles[0] == query_answer->handles[1]) {
        return;
    }

    string concept_[2] = {query_answer->handles[0], query_answer->handles[1]};
    vector<vector<string>> query;
    for (unsigned int i = 0; i < 2; i++) {
        // clang-format off
        query[i] = {
            OR_OPERATOR, "2"
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
        };
        // clang-format on
    }
    double count_0, count_1, count_intersection, count_union;
    QueryAnswerElement target_element(PREDICATE1);
    compute_counts(query, context, target_element, count_0, count_1, count_intersection, count_union);
    if (count_union > 0) {
        double strength = count_intersection / count_union;
        add_or_update_link(EQUIVALENCE_HANDLE, concept_[0], concept_[1], strength);
        add_or_update_link(EQUIVALENCE_HANDLE, concept_[1], concept_[0], strength);
    }
}

static void build_links(const vector<string>& query,
                        const string& context,
                        void (*build_link)(shared_ptr<QueryAnswer> query_answer, const string& context)) {
    auto proxy = issue_query(query, context, LINK_BUILDING_QUERY_SIZE);
    unsigned int count = 0;
    shared_ptr<QueryAnswer> query_answer;
    while (!proxy->finished()) {
        if ((query_answer = proxy->pop()) == NULL) {
            Utils::sleep();
        } else {
            if (++count <= LINK_BUILDING_QUERY_SIZE) {
                build_link(query_answer, context);
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

static void run(const string& predicate_source, const string& predicate_target, const string& context_tag) {
    // clang-format off

    vector<string> implication_query = {
        AND_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "2",
                NODE, SYMBOL, PREDICATE,
                VARIABLE, V1,
            LINK_TEMPLATE, EXPRESSION, "2",
                NODE, SYMBOL, PREDICATE,
                VARIABLE, V2,
    };

    vector<string> equivalence_query = {
        AND_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "2",
                NODE, SYMBOL, CONCEPT,
                VARIABLE, V1,
            LINK_TEMPLATE, EXPRESSION, "2",
                NODE, SYMBOL, CONCEPT,
                VARIABLE, V2,
    };

    vector<string> chain_query = {
        OR_OPERATOR, "3",
            AND_OPERATOR, "2",
                LINK_TEMPLATE, EXPRESSION, "2",
                    NODE, SYMBOL, IMPLICATION,
                    ATOM, predicate_source,
                    VARIABLE, PREDICATE1,
                LINK_TEMPLATE, EXPRESSION, "2",
                    NODE, SYMBOL, IMPLICATION,
                    VARIABLE, PREDICATE1,
                    ATOM, predicate_target,
            AND_OPERATOR, "3",
                LINK_TEMPLATE, EXPRESSION, "2",
                    NODE, SYMBOL, IMPLICATION,
                    ATOM, predicate_source,
                    VARIABLE, PREDICATE1,
                LINK_TEMPLATE, EXPRESSION, "2",
                    NODE, SYMBOL, IMPLICATION,
                    VARIABLE, PREDICATE1,
                    VARIABLE, PREDICATE2,
                LINK_TEMPLATE, EXPRESSION, "2",
                    NODE, SYMBOL, IMPLICATION,
                    VARIABLE, PREDICATE2,
                    ATOM, predicate_target,
            AND_OPERATOR, "4",
                LINK_TEMPLATE, EXPRESSION, "2",
                    NODE, SYMBOL, IMPLICATION,
                    ATOM, predicate_source,
                    VARIABLE, PREDICATE1,
                LINK_TEMPLATE, EXPRESSION, "2",
                    NODE, SYMBOL, IMPLICATION,
                    VARIABLE, PREDICATE1,
                    VARIABLE, PREDICATE2,
                LINK_TEMPLATE, EXPRESSION, "2",
                    NODE, SYMBOL, IMPLICATION,
                    VARIABLE, PREDICATE2,
                    VARIABLE, PREDICATE3,
                LINK_TEMPLATE, EXPRESSION, "2",
                    NODE, SYMBOL, IMPLICATION,
                    VARIABLE, PREDICATE3,
                    ATOM, predicate_target,
    };

    vector<string> context_query = {
        LINK_TEMPLATE, EXPRESSION, "3",
            VARIABLE, V1,
            VARIABLE, V2,
            VARIABLE, V3,
    };

    vector<vector<string>> correlation_query_template = {{
        OR_OPERATOR, "3",
            AND_OPERATOR, "4",
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, EVALUATION,
                    VARIABLE, PREDICATE1,
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
                    VARIABLE, V1,
                    VARIABLE, SENTENCE2,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, IMPLICATION,
                VARIABLE, PREDICATE1,
                VARIABLE, V1,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, IMPLICATION,
                VARIABLE, V1,
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
    QueryAnswerElement v1(V1);

    vector<map<string, QueryAnswerElement>> correlation_query_constants = {
        {{PREDICATE1, predicate1}}/*,
        {{PREDICATE1, predicate1}, {PREDICATE2, predicate2}},
        {{PREDICATE1, predicate1}, {PREDICATE2, predicate2}, {PREDICATE3, predicate3}},*/
    };

    vector<vector<pair<QueryAnswerElement, QueryAnswerElement>>> correlation_mapping = {
        {{predicate1, v1}}/*,
        {{predicate1, v1}, {predicate2, v1}},
        {{predicate1, v1}, {predicate2, v1}, {predicate3, v1}},*/
    };

    /*******************************************************************
    // NOTE TO REVISOR: I left these queries in comments below just to keep
    // all queries in the same place to ease reference. They can't be pre-defined
    // like the other ones because they both contain a dynamic component.

    build_implication = {
        OR_OPERATOR, "2"
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
    };

    build_equivalence = {
        OR_OPERATOR, "2"
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
    };

    Toplevel expressions (-> created -- knowledge base):

    ->  (IMPLICATION (PREDICATE "feature_contains_adc_cbc") (PREDICATE "feature_contains_ccc_cac"))
    ->  (EQUIVALENCE (CONCEPT (Sentence "dca cbc bde aaa adc") (CONCEPT (Sentence "aaa cac acc baa dad")))
    --  (EVALUATION (PREDICATE "feature_contains_adc_cbc") (CONCEPT (Sentence "dca cbc bde aaa adc")))
    --  (Contains (Sentence "cdb bce dad cec aae") (Word "bce"))

    Target elements (-> independent):

    ->  (PREDICATE "feature_contains_ccc_cac")
    ->  (CONCEPT (Sentence "dca cbc bde aaa adc"))
        (IMPLICATION (PREDICATE "feature_contains_adc_cbc") (PREDICATE "feature_contains_ccc_cac"))
        (EQUIVALENCE (CONCEPT (Sentence "dca cbc bde aaa adc") (CONCEPT (Sentence "aaa cac acc baa dad")))
        (EVALUATION (PREDICATE "feature_contains_adc_cbc") (CONCEPT (Sentence "dca cbc bde aaa adc")))
    ********************************************************************/
    // clang-format on

    LOG_INFO("Setting up context");
    AtomSpace atom_space;
    QueryAnswerElement target2(V2);
    QueryAnswerElement target3(V3);
    QueryAnswerElement toplevel_link(0);
    auto context_obj = atom_space.create_context(
        context_tag, context_query, {{toplevel_link, target2}, {toplevel_link, target3}}, {});
    string context = context_obj->get_key();
    LOG_INFO("Context " + context + " is ready");

    for (unsigned int i = 0; i < NUM_ITERATIONS; i++) {
        build_links(implication_query, context, build_implication_link);
        build_links(equivalence_query, context, build_equivalence_link);
        AttentionBrokerClient::set_determiners(buffer_determiners, context);
        buffer_determiners.clear();
        evolve_chain_query(chain_query,
                           correlation_query_template,
                           correlation_query_constants,
                           correlation_mapping[0],
                           context);
    }
}

int main(int argc, char* argv[]) {
    // clang-format off
    if (argc != 15) {
        cerr << "Usage: " << argv[0]
             << " <client id> <server id> <start_port:end_port> <context_tag>"
                " <source_predicate> <target_predicate>"
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
    Node source_predicate_symbol(SYMBOL, source_predicate);
    Node target_predicate_symbol(SYMBOL, target_predicate);
    Link source(EXPRESSION, {predicate_symbol.handle(), source_predicate_symbol.handle()});
    Link target(EXPRESSION, {predicate_symbol.handle(), target_predicate_symbol.handle()});
    run(source.handle(), target.handle(), context_tag);

    return 0;
}
