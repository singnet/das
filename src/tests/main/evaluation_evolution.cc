#include <fstream>

#include "AndTwoPredicates.h"
#include "AtomDBAPITypes.h"
#include "AtomDBInitializer.h"
#include "AtomDBSingleton.h"
#include "AtomDBUtils.h"
#include "AttentionBrokerClient.h"
#include "CustomizableLinkCreator.h"
#include "FitnessFunctionRegistry.h"
#include "Hasher.h"
#include "JsonConfigParser.h"
#include "LinkCreationProxy.h"
#include "LinkCreatorRegistry.h"
#include "Logger.h"
#include "MettaParser.h"
#include "QueryAnswer.h"
#include "QueryEvolutionProxy.h"
#include "RemoteAtomDB.h"
#include "ServiceBusSingleton.h"
#include "SystemParametersSingleton.h"
#include "Utils.h"
#include "commons/atoms/MettaParserActions.h"
#include "tags.h"

// Variables
#define V1 "V1"
#define V2 "V2"
#define V3 "V3"

// Misc
#define FITNESS_FUNCTION "inference_toy"

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

static double ATTENTION_FOCUS_STRICTNESS = 0.10;
static unsigned int RANDOM_SEED = 1236;

static string PRESET_LINKS_FILE_PREFIX = "/opt/das/_PRESET_LINKS_";
static string PRESET_LINKS_FILE = PRESET_LINKS_FILE_PREFIX;
static string CONTEXT_FILE_NAME_PREFIX = "/opt/das/_CONTEXT_DUMP_";

using namespace std;
using namespace atomdb;
using namespace commons;
using namespace query_engine;
using namespace evolution;
using namespace service_bus;
using namespace attention_broker;
using namespace link_creation_agent;

enum ContextTaskType { UNDEFINED = 0, DETERMINER, CORRELATION, ACTIVATION };

static shared_ptr<AtomDB> db;
static HandleDecoder* DECODER;
static shared_ptr<ServiceBus> bus;
static vector<pair<shared_ptr<QueryAnswer>, unsigned int>> recorded_answers;

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

static shared_ptr<LinkCreationProxy> create_lca_proxy(
    const vector<string>& query_tokens,
    const string& context,
    const string& link_creator_tag,
    LinkCreator& link_creator,
    BaseProxy::ORCHESTRATION_SCHEMA_TYPE orchestration) {
    auto proxy = make_shared<LinkCreationProxy>(query_tokens, context, link_creator_tag, orchestration);
    proxy->parameters[LinkCreationProxy::LINK_CREATOR_EXTRA_PARAMETERS] =
        (string) link_creator.extra_parameters();
    proxy->parameters[LinkCreationProxy::MAX_SUCCESSFUL_CREATION_PER_ROUND] = (unsigned int) 10;
    proxy->parameters[LinkCreationProxy::MAX_UNPRODUCTIVE_VISITS_PER_ROUND] = (unsigned int) 500;
    proxy->parameters[LinkCreationProxy::MAX_VISIT_ATTEMPTS_PER_ROUND] = (unsigned int) 20;
    proxy->parameters[LinkCreationProxy::MAX_ROUNDS] = (unsigned int) 0;
    proxy->parameters[LinkCreationProxy::LINK_CREATION_STRENGTH_THRESHOLD] = (double) 0.1;
    proxy->parameters[LinkCreationProxy::LINK_CREATION_LOG_FILE_NAME] = (string) "_new_links.txt";
    proxy->parameters[LinkCreationProxy::LOG_NEW_LINKS] = (bool) true;
    proxy->parameters[LinkCreationProxy::LINK_CREATION_SPREAD_ACTIVATION] = (bool) true;
    proxy->parameters[PatternMatchingQueryProxy::MAX_ANSWERS] = (unsigned int) 0;
    proxy->parameters[PatternMatchingQueryProxy::DISREGARD_IMPORTANCE_FLAG] = (bool) false;
    proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = (bool) true;
    proxy->parameters[PatternMatchingQueryProxy::UNIQUE_VALUE_FLAG] = (bool) true;
    proxy->parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] = true;
    proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] = false;
    proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = (bool) true;
    proxy->parameters[BaseQueryProxy::ALLOW_INCOMPLETE_CHAIN_PATH] = (bool) false;
    proxy->parameters[BaseQueryProxy::ATTENTION_CORRELATION] = (unsigned int) BaseQueryProxy::NONE;
    proxy->parameters[BaseQueryProxy::ATTENTION_UPDATE] = (unsigned int) BaseQueryProxy::NONE;
    proxy->parameters[BaseQueryProxy::ATTENTION_FOCUS_STRICTNESS] = (double) ATTENTION_FOCUS_STRICTNESS;
    return proxy;
}

static shared_ptr<PatternMatchingQueryProxy> issue_sti_snapshot_query(const vector<string>& query_tokens,
                                                                      const string& context) {
    auto proxy = make_shared<PatternMatchingQueryProxy>(query_tokens, context);
    proxy->parameters[PatternMatchingQueryProxy::MAX_ANSWERS] = (unsigned int) 0;
    proxy->parameters[PatternMatchingQueryProxy::DISREGARD_IMPORTANCE_FLAG] = (bool) false;
    proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = (bool) true;
    proxy->parameters[PatternMatchingQueryProxy::UNIQUE_VALUE_FLAG] = (bool) true;
    proxy->parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] = true;
    proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] = false;
    proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = (bool) true;
    ServiceBusSingleton::get_instance()->issue_bus_command(proxy);
    return proxy;
}

static void flush_remote_link_template_cache(bool force = false) {
    if (auto remote_db = dynamic_pointer_cast<RemoteAtomDB>(db)) {
        auto link_schema =
            LinkSchema({LINK_TEMPLATE, EXPRESSION, "3", VARIABLE, "V1", VARIABLE, "V2", VARIABLE, "V3"});
        remote_db->release_caches(link_schema, true, force);
    }
}

// clang-format off
static shared_ptr<QueryEvolutionProxy> query_evolution(
    const vector<string>& query_to_evolve,
    const vector<vector<string>>& correlation_query_template,
    unsigned int iteration,
    const string& context) {

    STACK_TRACE();
    QueryAnswerElement qa_predicate(PREDICATE);
    QueryAnswerElement qa_concept(CONCEPT);
    QueryAnswerElement qa_path1(0, 1, 2, false, false, true);
    QueryAnswerElement qa_path2(1, 1, 2, false, false, true);
    QueryAnswerElement qa_nothing;
    QueryAnswerElement qa_everything(QueryAnswerElement::EVERYTHING);

    vector<map<string, QueryAnswerElement>> correlation_query_constants = {
        {{V1, qa_predicate}},
        {{V2, qa_concept}}
    };
    vector<vector<pair<QueryAnswerElement, QueryAnswerElement>>> correlation_mapping = {
        {{qa_concept, qa_concept}, {qa_path1, qa_nothing}, {qa_path2, qa_nothing}},
        {{qa_predicate, qa_predicate}, {qa_path1, qa_nothing}, {qa_path2, qa_nothing}}
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
    proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] = false;
    proxy->parameters[BaseQueryProxy::ALLOW_INCOMPLETE_CHAIN_PATH] = true;
    proxy->parameters[BaseQueryProxy::MAX_BUNDLE_SIZE] = (unsigned int) 1000;
    proxy->parameters[BaseQueryProxy::ATTENTION_FOCUS_STRICTNESS] = (double) 0.0;
    proxy->parameters[PatternMatchingQueryProxy::DISREGARD_IMPORTANCE_FLAG] = false;
    proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = true;
    proxy->parameters[PatternMatchingQueryProxy::UNIQUE_VALUE_FLAG] = false;
    proxy->parameters[PatternMatchingQueryProxy::COUNT_FLAG] = false;
    proxy->parameters[QueryEvolutionProxy::POPULATION_SIZE] = (unsigned int) POPULATION_SIZE;
    proxy->parameters[QueryEvolutionProxy::MAX_GENERATIONS] = (unsigned int) MAX_GENERATIONS; // XXXXX
    proxy->parameters[QueryEvolutionProxy::ELITISM_RATE] = (double) ELITISM_RATE; // XXXXX
    proxy->parameters[QueryEvolutionProxy::SELECTION_RATE] = (double) SELECTION_RATE;

    bus->issue_bus_command(proxy);
    return proxy;
}

static void add_preset_links(const string& context) {
    STACK_TRACE();
    map<string, unsigned int> stimuli;
    vector<vector<string>> buffer_determiners;
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
            string link_handle = link->handle();
            double strength = (double) Utils::string_to_float(line[0]);
            unsigned int stimulus = (unsigned int) std::lround(strength * 1000);
            for (string& target_handle : link->targets) {
                if ((stimuli.find(target_handle) == stimuli.end()) || (stimuli[target_handle] < stimulus)) {
                    stimuli[target_handle] = stimulus;
                }
            }
            link->custom_attributes["strength"] = strength;
            LOG_INFO("Adding initial Link: [" + line[0] + "] " + line[1]);
            vector<Atom*> atoms_to_add;
            atoms_to_add.reserve(parser_handler->handle_to_atom.size());
            for (const auto& [_, atom] : parser_handler->handle_to_atom) {
                atoms_to_add.push_back(atom.get());
            }
            db->add_atoms(atoms_to_add, true);
            count++;
            line.clear();
            buffer_determiners.push_back({link_handle, link->targets[1], link->targets[2]});
            AttentionBrokerClient::correlate({link->targets[1], link->targets[2]}, context);
        }
        LOG_INFO(std::to_string(count) + " preset links read.");
    } else {
        RAISE_ERROR("Couldn't open file: " + PRESET_LINKS_FILE);
    }
    file.close();
    LOG_INFO("Updating determiners in AttentionBroker");
    AttentionBrokerClient::set_determiners(buffer_determiners, context);
    AttentionBrokerClient::stimulate(stimuli, context);
    buffer_determiners.clear();
    flush_remote_link_template_cache();
}

static vector<string> make_implication_query(bool target_flag=false) {
    return {
        AND_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION_TAG,
                VARIABLE, PREDICATE1,
                VARIABLE, CONCEPT,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION_TAG,
                (target_flag ? ATOM : VARIABLE), (target_flag ? TARGET_PREDICATE_HANDLE : PREDICATE2),
                VARIABLE, CONCEPT
    };
}

/*
static string make_implication_count_query(const string& _predicate) {
    return Utils::join({
        OR_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION_TAG,
                ATOM, _predicate,
                VARIABLE, CONCEPT1,
            AND_OPERATOR, "2",
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, EVALUATION_TAG,
                    ATOM, _predicate,
                    VARIABLE, CONCEPT2,
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, EQUIVALENCE_TAG,
                    VARIABLE, CONCEPT2,
                    VARIABLE, CONCEPT1
    });
}
*/

static string make_implication_count_query(const string& _predicate) {
    return Utils::join({
        LINK_TEMPLATE, EXPRESSION, "3",
            NODE, SYMBOL, EVALUATION_TAG,
            ATOM, _predicate,
            VARIABLE, CONCEPT1,
    });
}

static vector<string> make_equivalence_query(bool target_flag=false) {
    return {
        AND_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION_TAG,
                VARIABLE, PREDICATE,
                VARIABLE, CONCEPT1,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION_TAG,
                VARIABLE, PREDICATE,
                (target_flag ? ATOM : VARIABLE), (target_flag ? TARGET_CONCEPT_HANDLE : CONCEPT2)
    };
}

/*
static string make_equivalence_count_query(const string& _concept) {
    return Utils::join({
        OR_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION_TAG,
                VARIABLE, PREDICATE1,
                ATOM, _concept,
            AND_OPERATOR, "2",
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, EVALUATION_TAG,
                    VARIABLE, PREDICATE2,
                    ATOM, _concept,
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, IMPLICATION_TAG,
                    VARIABLE, PREDICATE2,
                    VARIABLE, PREDICATE1
    });
}
*/

static string make_equivalence_count_query(const string& _concept) {
    return Utils::join({
        LINK_TEMPLATE, EXPRESSION, "3",
            NODE, SYMBOL, EVALUATION_TAG,
            VARIABLE, PREDICATE1,
            ATOM, _concept,
    });
}

static vector<string> make_evaluation_predicate_query() {
    return {
        AND_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION_TAG,
                VARIABLE, PREDICATE,
                VARIABLE, CONCEPT1,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EQUIVALENCE_TAG,
                VARIABLE, CONCEPT1,
                VARIABLE, CONCEPT
    };
}

static vector<string> make_evaluation_concept_query() {
    return {
        AND_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION_TAG,
                VARIABLE, PREDICATE1,
                VARIABLE, CONCEPT,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, IMPLICATION_TAG,
                VARIABLE, PREDICATE1,
                VARIABLE, PREDICATE,
    };
}

static vector<string> make_plain_evaluation_query() {
    return {
        LINK_TEMPLATE, EXPRESSION, "3",
            NODE, SYMBOL, EVALUATION_TAG,
            VARIABLE, PREDICATE,
            VARIABLE, CONCEPT,
    };
}

static vector<string> make_plain_implication_query() {
    return {
        LINK_TEMPLATE, EXPRESSION, "3",
            NODE, SYMBOL, IMPLICATION_TAG,
            VARIABLE, PREDICATE1,
            VARIABLE, PREDICATE2,
    };
}

static vector<string> make_plain_equivalence_query() {
    return {
        LINK_TEMPLATE, EXPRESSION, "3",
            NODE, SYMBOL, EQUIVALENCE_TAG,
            VARIABLE, CONCEPT1,
            VARIABLE, CONCEPT2,
    };
}

// clang-format on

static void sti_snapshot(const string& context, bool report_evaluation = false) {
    float sti_threshold = 0.001;
    // float sti_threshold = 0.0;
    unsigned int index_count = 0;
    unsigned int predicate_index = index_count++;
    unsigned int concept_index = index_count++;
    unsigned int implication_index = index_count++;
    unsigned int equivalence_index = index_count++;
    unsigned int evaluation_index = index_count++;
    map<unsigned int, string> tag;
    tag[predicate_index] = "PREDICATES";
    tag[concept_index] = "CONCEPTS";
    tag[implication_index] = "IMPLICATION LINKS";
    tag[equivalence_index] = "EQUIVALENCE LINKS";
    tag[evaluation_index] = "EVALUATION LINKS";
    vector<set<string>> handle_set = {{}, {}, {}, {}, {}};
    vector<vector<pair<string, float>>> data = {{}, {}, {}, {}, {}};
    vector<vector<string>> handles = {{}, {}, {}, {}, {}};
    vector<vector<float>> sti = {{}, {}, {}, {}, {}};
    shared_ptr<QueryAnswer> query_answer;
    shared_ptr<PatternMatchingQueryProxy> proxy;

    proxy = issue_sti_snapshot_query(make_plain_evaluation_query(), context);
    while (true) {
        if ((query_answer = proxy->pop()) == nullptr) {
            if (proxy->finished()) {
                break;
            }
            Utils::sleep();
        } else {
            handle_set[predicate_index].insert(query_answer->get(QueryAnswerElement(PREDICATE)));
            handle_set[concept_index].insert(query_answer->get(QueryAnswerElement(CONCEPT)));
            handle_set[evaluation_index].insert(query_answer->get(QueryAnswerElement(0)));
        }
    }

    proxy = issue_sti_snapshot_query(make_plain_implication_query(), context);
    while (true) {
        if ((query_answer = proxy->pop()) == nullptr) {
            if (proxy->finished()) {
                break;
            }
            Utils::sleep();
        } else {
            handle_set[implication_index].insert(query_answer->get(QueryAnswerElement(0)));
        }
    }

    proxy = issue_sti_snapshot_query(make_plain_equivalence_query(), context);
    while (true) {
        if ((query_answer = proxy->pop()) == nullptr) {
            if (proxy->finished()) {
                break;
            }
            Utils::sleep();
        } else {
            handle_set[equivalence_index].insert(query_answer->get(QueryAnswerElement(0)));
        }
    }

    LOG_INFO("................................................................................");
    LOG_INFO("STI Snapshot");
    LOG_INFO("");
    for (unsigned int i = 0; i < index_count; i++) {
        LOG_INFO(to_string(handle_set[i].size()) + " " + tag[i]);
    }
    LOG_INFO("");
    LOG_INFO("Listing only links with STI >= " + to_string(sti_threshold));
    for (unsigned int i = 0; i < index_count; i++) {
        if (!report_evaluation && (i == evaluation_index)) {
            break;
        }
        LOG_INFO("");
        LOG_INFO(tag[i]);
        LOG_INFO("");
        handles[i] = vector<string>(handle_set[i].begin(), handle_set[i].end());
        AttentionBrokerClient::get_importance(handles[i], context, sti[i]);
        if (handles[i].size() != sti[i].size()) {
            RAISE_ERROR("Invalid attention broker answer. " + to_string(handle_set[i].size()) +
                        " handles has been queried and " + to_string(sti[i].size()) +
                        " STI values has returned.");
        }
        for (unsigned int j = 0; j < handles[i].size(); j++) {
            data[i].push_back(make_pair(handles[i][j], sti[i][j]));
        }
        std::sort(data[i].begin(),
                  data[i].end(),
                  [](const std::pair<string, float>& left, const std::pair<string, float>& right) {
                      return left.second > right.second;
                  });
        for (unsigned int j = 0; j < data[i].size(); j++) {
            if (data[i][j].second >= sti_threshold) {
                LOG_INFO("[" + to_string(data[i][j].second) + "] " +
                         AtomDBUtils::handle_to_metta(data[i][j].first));
            }
        }
    }
    LOG_INFO("................................................................................");
}

static vector<shared_ptr<LinkCreationProxy>> one_shot_lca_proxy;
// TODO XXX FIXME
// start_one_shot_lca ignores handle in its query and always builds implication links.
//
// make_implication_query(true) always binds the second evaluation to
// TARGET_PREDICATE_HANDLE. The function does not use handle in the query.
// handle appears only in the link specification and in the second count query.
// This causes two problems:
//
// start_one_shot_lca(TARGET_CONCEPT_HANDLE, context, true) at Line 760 creates
// IMPLICATION links of the form Predicate1 -> <target concept>. The candidates
// come from co-occurrence with the target predicate. A concept target needs an
// EQUIVALENCE link and a concept-based query.
//
// For hop handles at Line 844, candidate predicates are still selected by
// co-occurrence with the target predicate, not with handle.
//
// equivalence_to_target_link_creator and implication_to_target_link_creator are
// configured at Lines 692-705 but never used. This suggests the concept case
// was meant to use the equivalence creator. Pass the handle into the query
// builder, and pick the creator by target kind.
void start_one_shot_lca(const string& handle, const string& context, bool initial_links = false) {
    CustomizableLinkCreator implication_to_handle_link_creator(true);
    implication_to_handle_link_creator.add_link_specification(
        {QueryAnswerElement(PREDICATE1), QueryAnswerElement(handle, true)},
        {QueryAnswerElement(CONCEPT1), QueryAnswerElement(CONCEPT1)},
        IMPLICATION_TAG,
        CustomizableLinkCreator::INTERSECTION_OVER_A,
        {make_implication_count_query("QueryAnswerElement($Predicate1)"),
         make_implication_count_query("QueryAnswerElement(#" + handle + ")")});
    BaseProxy::ORCHESTRATION_SCHEMA_TYPE orchestration =
        (initial_links ? BaseProxy::SYNC_ON_CYCLE_START : BaseProxy::NONE);
    shared_ptr<LinkCreationProxy> implication_to_handle =
        create_lca_proxy(make_implication_query(true),
                         context,
                         LinkCreatorRegistry::CUSTOMIZABLE,
                         implication_to_handle_link_creator,
                         orchestration);
    implication_to_handle->parameters[LinkCreationProxy::MAX_ROUNDS] = (unsigned int) 1;
    if (initial_links) {
        implication_to_handle->parameters[LinkCreationProxy::MAX_SUCCESSFUL_CREATION_PER_ROUND] =
            (unsigned int) 100;
        implication_to_handle->parameters[LinkCreationProxy::MAX_UNPRODUCTIVE_VISITS_PER_ROUND] =
            (unsigned int) 100000;
        implication_to_handle->parameters[LinkCreationProxy::MAX_VISIT_ATTEMPTS_PER_ROUND] =
            (unsigned int) 100000;
        implication_to_handle->parameters[PatternMatchingQueryProxy::DISREGARD_IMPORTANCE_FLAG] =
            (bool) true;
        implication_to_handle->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] =
            (bool) false;
        implication_to_handle->parameters[BaseQueryProxy::ATTENTION_FOCUS_STRICTNESS] = (double) 0.0;
        implication_to_handle->parameters[LinkCreationProxy::LINK_CREATION_LOG_FILE_NAME] =
            (string) "_initial_links.txt";
    }
    one_shot_lca_proxy.push_back(implication_to_handle);
    ServiceBusSingleton::get_instance()->issue_bus_command(implication_to_handle);
    Utils::sleep(2000);  // XXXXX
    if (initial_links) {
        implication_to_handle->allow_cycle_start();
        Utils::sleep(2000);  // XXXXX
        while (!implication_to_handle->finished_cycle(true)) {
            Utils::sleep();
        }
    }
}

static void run(const string& context_tag) {
    STACK_TRACE();

    string context = Hasher::context_handle(context_tag);
    LOG_INFO("Context: " + context_tag + " -> " + context);
    LOG_INFO("Setting up STI in attention broker by stimulating only: " + TARGET_PREDICATE +
             " Handle: " + TARGET_PREDICATE_HANDLE + " and " + TARGET_CONCEPT +
             " Handle: " + TARGET_CONCEPT_HANDLE);
    AttentionBrokerClient::stimulate({{TARGET_PREDICATE_HANDLE, 1}, {TARGET_CONCEPT_HANDLE, 1}},
                                     context);
    PRESET_LINKS_FILE = PRESET_LINKS_FILE_PREFIX + context + ".txt";
    string context_file_name = CONTEXT_FILE_NAME_PREFIX + context + ".txt";
    filesystem::path context_file_path = context_file_name;
    LOG_INFO("Setting up AttentionBroker context: " + context_file_name);
    if (!filesystem::exists(context_file_path)) {
        RAISE_ERROR("Context file doesn't exist");
    }
    AttentionBrokerClient::drop_and_load_context(context, string(context_file_path));
    LOG_INFO("Context " + context + " is ready");

    // clang-format off
    vector<string> query_to_evolve = {
        OR_OPERATOR, "3",
            AND_OPERATOR, "2",
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, EVALUATION_TAG,
                    VARIABLE, PREDICATE,
                    ATOM, TARGET_CONCEPT_HANDLE,
                CHAIN_OPERATOR, "0", "1", "2",
                    VARIABLE, PREDICATE,
                    ATOM, TARGET_PREDICATE_HANDLE,
                    LINK_TEMPLATE, EXPRESSION, "3",
                        NODE, SYMBOL, IMPLICATION_TAG,
                        VARIABLE, PREDICATE1,
                        VARIABLE, PREDICATE2,
            AND_OPERATOR, "2",
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, EVALUATION_TAG,
                    ATOM, TARGET_PREDICATE_HANDLE,
                    VARIABLE, CONCEPT,
                CHAIN_OPERATOR, "0", "1", "2",
                    VARIABLE, CONCEPT,
                    ATOM, TARGET_CONCEPT_HANDLE,
                    LINK_TEMPLATE, EXPRESSION, "3",
                        NODE, SYMBOL, EQUIVALENCE_TAG,
                        VARIABLE, CONCEPT1,
                        VARIABLE, CONCEPT2,
            AND_OPERATOR, "3",
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, EVALUATION_TAG,
                    VARIABLE, PREDICATE,
                    VARIABLE, CONCEPT,
                CHAIN_OPERATOR, "0", "1", "2",
                    VARIABLE, PREDICATE,
                    ATOM, TARGET_PREDICATE_HANDLE,
                    LINK_TEMPLATE, EXPRESSION, "3",
                        NODE, SYMBOL, IMPLICATION_TAG,
                        VARIABLE, PREDICATE1,
                        VARIABLE, PREDICATE2,
                CHAIN_OPERATOR, "0", "1", "2",
                    VARIABLE, CONCEPT,
                    ATOM, TARGET_CONCEPT_HANDLE,
                    LINK_TEMPLATE, EXPRESSION, "3",
                        NODE, SYMBOL, EQUIVALENCE_TAG,
                        VARIABLE, CONCEPT1,
                        VARIABLE, CONCEPT2,
    };
    /*
    vector<string> query_to_evolve = {
        AND_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION_TAG,
                VARIABLE, PREDICATE,
                ATOM, TARGET_CONCEPT_HANDLE,
            CHAIN_OPERATOR, "0", "1", "2",
                VARIABLE, PREDICATE,
                ATOM, TARGET_PREDICATE_HANDLE,
                LINK_TEMPLATE, EXPRESSION, "3",
                    NODE, SYMBOL, IMPLICATION_TAG,
                    VARIABLE, PREDICATE1,
                    VARIABLE, PREDICATE2,
    };
    */

    vector<vector<string>> correlation_query_template = {
        {LINK_TEMPLATE, EXPRESSION, "3",
            NODE, SYMBOL, EVALUATION_TAG,
            VARIABLE, V1,
            VARIABLE, CONCEPT},
        {LINK_TEMPLATE, EXPRESSION, "3",
            NODE, SYMBOL, EVALUATION_TAG,
            VARIABLE, PREDICATE,
            VARIABLE, V2}
    };
    // clang-format on

    AndTwoPredicates and_two_predicates;
    CustomizableLinkCreator implication_to_target_link_creator(true);
    CustomizableLinkCreator equivalence_to_target_link_creator;
    CustomizableLinkCreator implication_link_creator(true);
    CustomizableLinkCreator equivalence_link_creator;
    CustomizableLinkCreator evaluation_link_creator;

    // initial target atoms
    implication_to_target_link_creator.add_link_specification(
        {QueryAnswerElement(PREDICATE1), QueryAnswerElement(TARGET_PREDICATE_HANDLE, true)},
        {QueryAnswerElement(CONCEPT1), QueryAnswerElement(CONCEPT1)},
        IMPLICATION_TAG,
        CustomizableLinkCreator::INTERSECTION_OVER_A,
        {make_implication_count_query("QueryAnswerElement($Predicate1)"),
         make_implication_count_query("QueryAnswerElement(#" + TARGET_PREDICATE_HANDLE + ")")});
    equivalence_to_target_link_creator.add_link_specification(
        {QueryAnswerElement(CONCEPT1), QueryAnswerElement(CONCEPT2)},
        {QueryAnswerElement(PREDICATE1), QueryAnswerElement(PREDICATE1)},
        EQUIVALENCE_TAG,
        CustomizableLinkCreator::INTERSECTION_OVER_UNION,
        {make_equivalence_count_query("QueryAnswerElement($Concept1)"),
         make_equivalence_count_query("QueryAnswerElement(#" + TARGET_CONCEPT_HANDLE + ")")});

    // Implication and equivalence
    implication_link_creator.add_link_specification(
        {QueryAnswerElement(PREDICATE1), QueryAnswerElement(PREDICATE2)},
        {QueryAnswerElement(CONCEPT1), QueryAnswerElement(CONCEPT1)},
        IMPLICATION_TAG,
        CustomizableLinkCreator::INTERSECTION_OVER_A,
        {make_implication_count_query("QueryAnswerElement($Predicate1)"),
         make_implication_count_query("QueryAnswerElement($Predicate2)")});
    equivalence_link_creator.add_link_specification(
        {QueryAnswerElement(CONCEPT1), QueryAnswerElement(CONCEPT2)},
        {QueryAnswerElement(PREDICATE1), QueryAnswerElement(PREDICATE1)},
        EQUIVALENCE_TAG,
        CustomizableLinkCreator::INTERSECTION_OVER_UNION,
        {make_equivalence_count_query("QueryAnswerElement($Concept1)"),
         make_equivalence_count_query("QueryAnswerElement($Concept2)")});

    // Weak evaluations
    evaluation_link_creator.add_link_specification(
        {QueryAnswerElement(PREDICATE), QueryAnswerElement(CONCEPT)},
        {QueryAnswerElement(0), QueryAnswerElement(1)},
        EVALUATION_TAG,
        CustomizableLinkCreator::PRODUCT,
        {});

    // LOG_INFO("----- Adding initial links");
    // add_preset_links(context);
    // sti_snapshot(context, false); // XXXXX
    // exit(0); // XXXXX
    /*
    shared_ptr<LinkCreationProxy> implication_to_target = create_lca_proxy(make_implication_query(true),
    context, LinkCreatorRegistry::CUSTOMIZABLE, implication_to_target_link_creator,
    BaseProxy::SYNC_ON_CYCLE_START);
    implication_to_target->parameters[LinkCreationProxy::MAX_SUCCESSFUL_CREATION_PER_ROUND] = (unsigned
    int) 100; implication_to_target->parameters[LinkCreationProxy::MAX_UNPRODUCTIVE_VISITS_PER_ROUND] =
    (unsigned int) 100000;
    implication_to_target->parameters[LinkCreationProxy::MAX_VISIT_ATTEMPTS_PER_ROUND] = (unsigned int)
    100000; implication_to_target->parameters[PatternMatchingQueryProxy::DISREGARD_IMPORTANCE_FLAG] =
    (bool) true; implication_to_target->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] =
    (bool) false; implication_to_target->parameters[BaseQueryProxy::ATTENTION_FOCUS_STRICTNESS] =
    (double) 0.0; implication_to_target->parameters[LinkCreationProxy::LINK_CREATION_LOG_FILE_NAME] =
    (string) "_initial_links.txt";
    ServiceBusSingleton::get_instance()->issue_bus_command(implication_to_target);
    Utils::sleep(2000); // XXXXX
    implication_to_target->allow_cycle_start();
    while (!implication_to_target->finished_cycle(true)) {
        Utils::sleep();
    }
    */
    // LOG_INFO("--------------------------------------------------------------------------------");
    // query_evolution(query_to_evolve, correlation_query_template, 1, context); // XXXXX
    // exit(0); // XXXXX

    start_one_shot_lca(TARGET_PREDICATE_HANDLE, context, true);
    start_one_shot_lca(TARGET_CONCEPT_HANDLE, context, true);
    // sti_snapshot(context, false); // XXXXX
    // exit(0); // XXXXX

    shared_ptr<LinkCreationProxy> and2predicates =
        create_lca_proxy(make_implication_query(),
                         context,
                         LinkCreatorRegistry::AND_TWO_PREDICATES,
                         and_two_predicates,
                         BaseProxy::NONE);
    shared_ptr<LinkCreationProxy> implication = create_lca_proxy(make_implication_query(),
                                                                 context,
                                                                 LinkCreatorRegistry::CUSTOMIZABLE,
                                                                 implication_link_creator,
                                                                 BaseProxy::NONE);
    shared_ptr<LinkCreationProxy> equivalence = create_lca_proxy(make_equivalence_query(),
                                                                 context,
                                                                 LinkCreatorRegistry::CUSTOMIZABLE,
                                                                 equivalence_link_creator,
                                                                 BaseProxy::NONE);
    shared_ptr<LinkCreationProxy> evaluation_predicate =
        create_lca_proxy(make_evaluation_predicate_query(),
                         context,
                         LinkCreatorRegistry::CUSTOMIZABLE,
                         evaluation_link_creator,
                         BaseProxy::NONE);
    shared_ptr<LinkCreationProxy> evaluation_concept =
        create_lca_proxy(make_evaluation_concept_query(),
                         context,
                         LinkCreatorRegistry::CUSTOMIZABLE,
                         evaluation_link_creator,
                         BaseProxy::NONE);

    evaluation_predicate->parameters[LinkCreationProxy::LINK_CREATION_STRENGTH_THRESHOLD] =
        (double) 0.05;
    evaluation_predicate->parameters[LinkCreationProxy::LINK_CREATION_LOG_FILE_NAME] =
        (string) "_new_weak_evaluation_links.txt";
    evaluation_predicate->parameters[LinkCreationProxy::LINK_CREATION_SPREAD_ACTIVATION] = (bool) false;

    evaluation_concept->parameters[LinkCreationProxy::LINK_CREATION_STRENGTH_THRESHOLD] = (double) 0.05;
    evaluation_concept->parameters[LinkCreationProxy::LINK_CREATION_LOG_FILE_NAME] =
        (string) "_new_weak_evaluation_links.txt";
    evaluation_concept->parameters[LinkCreationProxy::LINK_CREATION_SPREAD_ACTIVATION] = (bool) false;

    ServiceBusSingleton::get_instance()->issue_bus_command(and2predicates);
    Utils::sleep(2000);    // XXXXX
    ServiceBusSingleton::get_instance()->issue_bus_command(implication);
    Utils::sleep(2000);    // XXXXX
    ServiceBusSingleton::get_instance()->issue_bus_command(equivalence);
    Utils::sleep(2000);    // XXXXX
    ServiceBusSingleton::get_instance()->issue_bus_command(evaluation_predicate);
    Utils::sleep(2000);    // XXXXX
    ServiceBusSingleton::get_instance()->issue_bus_command(evaluation_concept);
    Utils::sleep(2000);    // XXXXX

    Utils::sleep(180000);  // XXXXX
    // Utils::sleep(300000); // XXXXX

    double best_fitness = 0.0;
    for (unsigned int iteration = 1; iteration <= NUM_ITERATIONS; iteration++) {
        LOG_INFO("--------------------------------------------------------------------------------");
        LOG_INFO("Iteration " + to_string(iteration));
        LOG_INFO("--------------------------------------------------------------------------------");
        shared_ptr<QueryEvolutionProxy> qe_proxy =
            query_evolution(query_to_evolve, correlation_query_template, iteration, context);

        shared_ptr<QueryAnswer> query_answer;
        unsigned int count_answers = 0;
        set<string> hop_handles_to_process;

        while (!qe_proxy->finished()) {
            if ((query_answer = qe_proxy->pop()) == NULL) {
                Utils::sleep();
            } else {
                count_answers++;
                if (query_answer->strength > best_fitness) {
                    best_fitness = query_answer->strength;
                    LOG_INFO("ANSWER: " + answer_to_string(query_answer));
                    recorded_answers.push_back({query_answer, iteration});
                }
                for (unsigned int path_index = 0; path_index < query_answer->get_paths_size();
                     path_index++) {
                    QueryAnswerElement hops(path_index, 1, 2, false, false, false);
                    vector<string> hop_handles = query_answer->get_all(hops, db);
                    hop_handles_to_process.insert(hop_handles.begin(), hop_handles.end());
                }
            }
        }
        for (const string& handle : hop_handles_to_process) {
            start_one_shot_lca(handle, context);
        }
        LOG_INFO("Total answers in iteration " << iteration << ": " << count_answers);
        flush_remote_link_template_cache();
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
    vector<string> to_insert = {EQUIVALENCE_TAG, IMPLICATION_TAG, LOGICAL_AND_TAG};
    Node* node;
    for (string node_name : to_insert) {
        node = new Node(SYMBOL, node_name);
        db->add_node(node);
        delete (node);
    }
    flush_remote_link_template_cache();
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
    AtomDBInitializer::init(atomdb_config);

    Utils::init_random(RANDOM_SEED);
    db = AtomDBSingleton::get_instance();
    DECODER = static_pointer_cast<HandleDecoder>(db).get();
    ServiceBusSingleton::init(client_endpoint, server_endpoint, ports_range.first, ports_range.second);
    FitnessFunctionRegistry::initialize_statics();
    LinkCreatorRegistry::initialize_statics();
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

    flush_remote_link_template_cache(true);

    return 0;
}
