#include <fstream>

#include "AtomDBSingleton.h"
#include "AttentionBrokerClient.h"
#include "FitnessFunctionRegistry.h"
#include "Hasher.h"
#include "JsonConfigParser.h"
#include "Logger.h"
#include "MettaParser.h"
#include "LinkCreationProxy.h"
#include "LinkCreatorRegistry.h"
#include "CustomizableLinkCreator.h"
#include "AndTwoPredicates.h"
#include "tags.h"
#include "QueryAnswer.h"
#include "QueryEvolutionProxy.h"
#include "RemoteAtomDB.h"
#include "ServiceBusSingleton.h"
#include "SystemParametersSingleton.h"
#include "Utils.h"
#include "commons/atoms/MettaParserActions.h"

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

static double ATTENTION_FOCUS_STRICTNESS = 0.30;
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

static shared_ptr<LinkCreationProxy> issue_lca_query(
    const vector<string>& query_tokens,
    const string& context,
    const string& link_creator_tag,
    LinkCreator& link_creator,
    BaseProxy::ORCHESTRATION_SCHEMA_TYPE orchestration) {

    // orchestration = BaseProxy::NONE; // XXXXX
    auto proxy = make_shared<LinkCreationProxy>(query_tokens, context, link_creator_tag, orchestration);
    proxy->parameters[LinkCreationProxy::LINK_CREATOR_EXTRA_PARAMETERS] = (string) link_creator.extra_parameters();
    proxy->parameters[LinkCreationProxy::MAX_SUCCESSFUL_CREATION_PER_ROUND] = (unsigned int) 10;
    proxy->parameters[LinkCreationProxy::MAX_UNPRODUCTIVE_VISITS_PER_ROUND] = (unsigned int) 500;
    proxy->parameters[LinkCreationProxy::MAX_VISIT_ATTEMPTS_PER_ROUND] = (unsigned int) 10;
    proxy->parameters[LinkCreationProxy::MAX_ROUNDS] = (unsigned int) 0;
    proxy->parameters[LinkCreationProxy::LINK_CREATION_STRENGTH_THRESHOLD] = (double) 0.2; // 0.1;
    proxy->parameters[LinkCreationProxy::LINK_CREATION_LOG_FILE_NAME] = (string) "_new_links.txt";
    proxy->parameters[LinkCreationProxy::LOG_NEW_LINKS] = (bool) true;
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
static void query_evolution(
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
    proxy->parameters[BaseQueryProxy::ATTENTION_FOCUS_STRICTNESS] = (double) ATTENTION_FOCUS_STRICTNESS;
    proxy->parameters[PatternMatchingQueryProxy::DISREGARD_IMPORTANCE_FLAG] = false;
    proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = true;
    proxy->parameters[PatternMatchingQueryProxy::UNIQUE_VALUE_FLAG] = false;
    proxy->parameters[PatternMatchingQueryProxy::COUNT_FLAG] = false;
    proxy->parameters[QueryEvolutionProxy::POPULATION_SIZE] = (unsigned int) POPULATION_SIZE;
    proxy->parameters[QueryEvolutionProxy::MAX_GENERATIONS] = (unsigned int) MAX_GENERATIONS; // XXXXX
    proxy->parameters[QueryEvolutionProxy::ELITISM_RATE] = (double) ELITISM_RATE; // XXXXX
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

static vector<string> make_implication_query() {
    return {
        AND_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION_TAG,
                VARIABLE, PREDICATE1,
                VARIABLE, CONCEPT,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION_TAG,
                VARIABLE, PREDICATE2,
                VARIABLE, CONCEPT
    };
}

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

static vector<string> make_equivalence_query() {
    return {
        AND_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION_TAG,
                VARIABLE, PREDICATE,
                VARIABLE, CONCEPT1,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION_TAG,
                VARIABLE, PREDICATE,
                VARIABLE, CONCEPT2
    };
}

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
// clang-format on


static void add_preset_links(const string& context) {
    STACK_TRACE();
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
            link->custom_attributes["strength"] = (double) Utils::string_to_float(line[0]);
            LOG_DEBUG("Adding Link: [" + line[0] + "] " + line[1]);
            vector<Atom*> atoms_to_add;
            atoms_to_add.reserve(parser_handler->handle_to_atom.size());
            for (const auto& [_, atom] : parser_handler->handle_to_atom) {
                atoms_to_add.push_back(atom.get());
            }
            db->add_atoms(atoms_to_add, true);
            count++;
            line.clear();
            buffer_determiners.push_back({link->handle(), link->targets[1], link->targets[2]});
            AttentionBrokerClient::correlate({link->targets[1], link->targets[2]}, context);
        }
        LOG_INFO(std::to_string(count) + " preset links read.");
    } else {
        RAISE_ERROR("Couldn't open file: " + PRESET_LINKS_FILE);
    }
    file.close();
    LOG_INFO("Updating determiners in AttentionBroker");
    AttentionBrokerClient::set_determiners(buffer_determiners, context);
    buffer_determiners.clear();
    flush_remote_link_template_cache();
}

static void run(const string& context_tag) {
    STACK_TRACE();

    LOG_INFO("Setting up context for tag: " + context_tag);
    string context = Hasher::context_handle(context_tag);
    PRESET_LINKS_FILE = PRESET_LINKS_FILE_PREFIX + context + ".txt";
    filesystem::path context_file_name = CONTEXT_FILE_NAME_PREFIX + context + ".txt";
    if (!filesystem::exists(context_file_name)) {
        RAISE_ERROR("Context file doesn't exist");
    }
    LOG_INFO("Updating AttentionBroker");
    AttentionBrokerClient::drop_and_load_context(context, string(context_file_name));
    AttentionBrokerClient::stimulate({{TARGET_PREDICATE_HANDLE, 1}, {TARGET_CONCEPT_HANDLE, 1}}, context);
    LOG_INFO("Context " + context + " is ready");
    add_preset_links(context);

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
    CustomizableLinkCreator implication_link_creator;
    CustomizableLinkCreator equivalence_link_creator;
    CustomizableLinkCreator evaluation_link_creator;

    implication_link_creator.add_link_specification({QueryAnswerElement(PREDICATE1), QueryAnswerElement(PREDICATE2)},
                                                    {QueryAnswerElement(CONCEPT1), QueryAnswerElement(CONCEPT1)},
                                                    IMPLICATION_TAG,
                                                    CustomizableLinkCreator::INTERSECTION_OVER_A,
                                                    {make_implication_count_query("QueryAnswerElement($Predicate1)"), make_implication_count_query("QueryAnswerElement($Predicate2)")});
    equivalence_link_creator.add_link_specification({QueryAnswerElement(CONCEPT1), QueryAnswerElement(CONCEPT2)},
                                                    {QueryAnswerElement(PREDICATE1), QueryAnswerElement(PREDICATE1)},
                                                    EQUIVALENCE_TAG,
                                                    CustomizableLinkCreator::INTERSECTION_OVER_UNION,
                                                    {make_equivalence_count_query("QueryAnswerElement($Concept1)"), make_equivalence_count_query("QueryAnswerElement($Concept2)")});
    evaluation_link_creator.add_link_specification({QueryAnswerElement(PREDICATE), QueryAnswerElement(CONCEPT)},
                                                   {QueryAnswerElement(0), QueryAnswerElement(1)},
                                                   EVALUATION_TAG,
                                                   CustomizableLinkCreator::PRODUCT,
                                                   {});

    vector<shared_ptr<LinkCreationProxy>> lca_proxy = {
        issue_lca_query(make_implication_query(), context, LinkCreatorRegistry::AND_TWO_PREDICATES, and_two_predicates, BaseProxy::SYNC_ON_CYCLE_START),
        issue_lca_query(make_implication_query(), context, LinkCreatorRegistry::CUSTOMIZABLE, implication_link_creator, BaseProxy::SYNC_ON_CYCLE_START),
        issue_lca_query(make_equivalence_query(), context, LinkCreatorRegistry::CUSTOMIZABLE, equivalence_link_creator, BaseProxy::SYNC_ON_CYCLE_START),
        issue_lca_query(make_evaluation_predicate_query(), context, LinkCreatorRegistry::CUSTOMIZABLE, evaluation_link_creator, BaseProxy::SYNC_ON_CYCLE_START),
        issue_lca_query(make_evaluation_concept_query(), context, LinkCreatorRegistry::CUSTOMIZABLE, evaluation_link_creator, BaseProxy::SYNC_ON_CYCLE_START)
    };

    //NUM_ITERATIONS = 10; // XXXXX
    for (unsigned int iteration = 1; iteration <= NUM_ITERATIONS; iteration++) {
        LOG_INFO("--------------------------------------------------------------------------------");
        LOG_INFO("Iteration " + to_string(iteration));
        LOG_INFO("--------------------------------------------------------------------------------");
        LOG_INFO("----- Building links");
        AttentionBrokerClient::stimulate({{TARGET_PREDICATE_HANDLE, 1}, {TARGET_CONCEPT_HANDLE, 1}}, context);
        /*
        for (auto proxy : lca_proxy) {
            proxy->allow_cycle_start();
        }
        bool finished_flag = false;
        while (!finished_flag) {
            finished_flag = true;
            for (auto proxy : lca_proxy) {
                if (! proxy->finished_cycle(true)) {
                    finished_flag = false;
                    break;
                }
            }
            if (!finished_flag) {
                Utils::sleep();
            }
        }
        */
        for (auto proxy : lca_proxy) {
            proxy->allow_cycle_start();
            while (! proxy->finished_cycle(true)) {
                Utils::sleep();
            }
        }
        LOG_INFO("----- Evolving query");
        query_evolution(query_to_evolve, correlation_query_template, iteration, context);
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
    AtomDBSingleton::init(atomdb_config);

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
