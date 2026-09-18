#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "AtomDBAPITypes.h"
#include "AtomDBSingleton.h"
#include "ContextBrokerProxy.h"
#include "CountLetterFunction.h"
#include "FitnessFunctionRegistry.h"
#include "JsonConfigParser.h"
#include "Logger.h"
#include "QueryAnswer.h"
#include "ServiceBusSingleton.h"
#include "SystemParametersSingleton.h"
#include "Utils.h"
#include "httplib.h"
#include "nlohmann/json.hpp"

using namespace std;
using namespace atomdb;
using namespace commons;
using namespace query_engine;
using namespace service_bus;
using namespace context_broker;
using namespace fitness_functions;
using json = nlohmann::json;

static constexpr const char* FITNESS_FUNCTION = "count_letter";
static constexpr const char* SENTENCE_VAR = "sentence1";
static constexpr const char* WORD_VAR = "word1";
static constexpr const char* PLACEHOLDER_VAR = "placeholder1";

static unsigned int POPULATION_SIZE = 50;
static unsigned int MAX_GENERATIONS = 5;
static double ELITISM_RATE = 0.08;
static double SELECTION_RATE = 0.10;
static float RENT_RATE = 0.25;
static float SPREADING_RATE_LOWERBOUND = 0.50;
static float SPREADING_RATE_UPPERBOUND = 0.70;
static string HTTP_ENDPOINT;
static char LETTER_TO_COUNT = 'c';

static shared_ptr<AtomDB> db;

static vector<string> split_words(string s, const string& delimiter) {
    vector<string> tokens;
    size_t pos = 0;
    while ((pos = s.find(delimiter)) != string::npos) {
        tokens.push_back(s.substr(0, pos));
        s.erase(0, pos + delimiter.length());
    }
    tokens.push_back(s);
    return tokens;
}

static string highlight(const string& s, const set<string>& highlighted) {
    // Sentence names are typically quoted: "word1 word2 ..."
    if (s.size() < 2) {
        return s;
    }
    vector<string> tokens = split_words(s.substr(1, s.size() - 2), " ");
    string answer;
    for (size_t i = 0; i < tokens.size(); i++) {
        if (highlighted.find(tokens[i]) != highlighted.end()) {
            answer += "\033[1;4m" + tokens[i] + "\033[0m";
        } else {
            answer += tokens[i];
        }
        if (i + 1 < tokens.size()) {
            answer += " ";
        }
    }
    return answer;
}

static pair<string, int> split_host_port(const string& endpoint, int default_port) {
    auto colon = endpoint.rfind(':');
    if (colon == string::npos) {
        return {endpoint, default_port};
    }
    return {endpoint.substr(0, colon), Utils::string_to_int(endpoint.substr(colon + 1))};
}

static json metta_tokens_object(const string& expression) {
    return {{"syntax", "metta"}, {"tokens", json::array({expression})}};
}

static string sentence_name_from_answer(shared_ptr<QueryAnswer> answer) {
    string handle = answer->assignment.get(SENTENCE_VAR);
    auto sentence_link = db->get_link(handle);
    auto sentence_name_node = db->get_node(sentence_link->targets[1]);
    return sentence_name_node->name;
}

static void print_answer(shared_ptr<QueryAnswer> answer, const string& word_tag) {
    float fitness = static_cast<float>(answer->strength);
    string sentence_name = sentence_name_from_answer(answer);
    string highlighted = highlight(sentence_name, {word_tag});
    cout << std::fixed << std::setw(6) << std::setprecision(4) << std::setfill('0') << fitness << ": "
         << highlighted << endl;
}

/**
 * Notebook-equivalent evolution over HTTP:
 *   query:  (Contains $sentence1 (Word "<word>"))
 *   cq/cr/cm: Contains placeholder -> sentence1 -> word1
 *   fitness: remote_fitness_function on the wire; local count_letter on the client.
 */
static void run_http_evolution(const string& context_key, const string& word_tag) {
    const string query_expr = "(Contains $" + string(SENTENCE_VAR) + " (Word \"" + word_tag + "\"))";
    const string correlation_expr =
        "(Contains $" + string(PLACEHOLDER_VAR) + " $" + string(WORD_VAR) + ")";

    json body = {{"command", "evolution"},
                 {"params",
                  {{"evolution",
                    {{"query", metta_tokens_object(query_expr)},
                     {"fitness_function_tag", FitnessFunctionRegistry::REMOTE_FUNCTION},
                     {"correlation_queries", json::array({metta_tokens_object(correlation_expr)})},
                     {"correlation_replacements",
                      json::array({json::array({json::array({PLACEHOLDER_VAR, SENTENCE_VAR})})})},
                     {"correlation_mappings",
                      json::array({json::array({json::array({SENTENCE_VAR, WORD_VAR})})})}}},
                   {"context", context_key},
                   {"unique_assignment_flag", true},
                   {"populate_metta_mapping", true},
                   {"use_metta_as_query_tokens", true},
                   {"max_bundle_size", 10000},
                   {"positive_importance_flag", false},
                   {"disregard_importance_flag", false},
                   {"unique_value_flag", false},
                   {"count_flag", false},
                   {"population_size", POPULATION_SIZE},
                   {"max_generations", MAX_GENERATIONS},
                   {"elitism_rate", ELITISM_RATE},
                   {"selection_rate", SELECTION_RATE}}}};

    auto [host, port] = split_host_port(HTTP_ENDPOINT, 40009);
    httplib::Client http(host, port);
    http.set_connection_timeout(5);
    http.set_read_timeout(600);

    LOG_INFO("POST evolution to http://" + host + ":" + to_string(port) + " query=" + query_expr);
    auto create = http.Post("/command-router/executions", body.dump(), "application/json");
    if (!create || create->status != 202) {
        string detail = create ? create->body : "no response";
        RAISE_ERROR("HTTP evolution create failed: " + detail);
    }
    const string execution_id = json::parse(create->body)["execution_id"].get<string>();
    LOG_INFO("HTTP evolution execution_id=" + execution_id);

    httplib::ws::WebSocketClient ws("ws://" + host + ":" + to_string(port) + "/command-router/ws/" +
                                    execution_id);
    if (!ws.is_valid() || !ws.connect()) {
        RAISE_ERROR("HTTP evolution WebSocket connect failed for " + execution_id);
    }
    ws.set_read_timeout(600, 0);

    auto fitness_fn = FitnessFunctionRegistry::function(FITNESS_FUNCTION);
    unsigned int count = 0;
    string terminal_status;
    string msg;

    while (ws.read(msg)) {
        auto event = json::parse(msg);
        const string command = event.value("command", "");
        if (command == "eval_fitness") {
            const int seq = event["params"]["seq"].get<int>();
            const json& answers = event["params"]["answers"];
            json fitness_values = json::array();
            for (const auto& answer_json : answers) {
                auto answer = make_shared<QueryAnswer>();
                answer->from_json(answer_json);
                fitness_values.push_back(fitness_fn->eval(answer));
            }
            json response = {
                {"command", "eval_fitness_response"},
                {"params", {{"execution_id", execution_id}, {"seq", seq}, {"fitness", fitness_values}}}};
            if (!ws.send(response.dump())) {
                RAISE_ERROR("Failed to send eval_fitness_response");
            }
        } else if (command == "query_answers") {
            for (const auto& answer_json : event["params"]["answers"]) {
                auto answer = make_shared<QueryAnswer>();
                answer->from_json(answer_json);
                print_answer(answer, word_tag);
                count++;
            }
        } else if (command == "execution_status") {
            terminal_status = event["params"].value("status", "");
            if (terminal_status == "completed" || terminal_status == "error" ||
                terminal_status == "aborted") {
                if (terminal_status == "error") {
                    string message = event["params"].value("message", "unknown error");
                    ws.close();
                    RAISE_ERROR("HTTP evolution failed: " + message);
                }
                break;
            }
        }
    }
    ws.close();

    if (terminal_status != "completed") {
        RAISE_ERROR("HTTP evolution ended with status: " + terminal_status);
    }
    if (count == 0) {
        cout << "No match for query" << endl;
    } else {
        cout << "Count: " << count << endl;
    }
}

static string create_context(shared_ptr<ServiceBus> bus, const string& context_tag) {
    // Notebook: (Contains $sentence1 $word1) with determiner (0 -> sentence1), (sentence1 -> word1)
    // clang-format off
    vector<string> contains_query = {
        "LINK_TEMPLATE", "Expression", "3",
            "NODE", "Symbol", "Contains",
            "VARIABLE", SENTENCE_VAR,
            "VARIABLE", WORD_VAR,
    };
    // clang-format on

    QueryAnswerElement sentence_link(SENTENCE_VAR);
    QueryAnswerElement word_link(WORD_VAR);
    QueryAnswerElement contains_link(0);
    vector<pair<QueryAnswerElement, QueryAnswerElement>> determiner_schema = {
        {contains_link, sentence_link}, {sentence_link, word_link}};
    vector<QueryAnswerElement> stimulus_schema;

    auto context_proxy =
        make_shared<ContextBrokerProxy>(context_tag, contains_query, determiner_schema, stimulus_schema);
    context_proxy->parameters[ContextBrokerProxy::USE_CACHE] = true;
    context_proxy->parameters[ContextBrokerProxy::ENFORCE_CACHE_RECREATION] = false;
    context_proxy->parameters[ContextBrokerProxy::INITIAL_RENT_RATE] = static_cast<double>(RENT_RATE);
    context_proxy->parameters[ContextBrokerProxy::INITIAL_SPREADING_RATE_LOWERBOUND] =
        static_cast<double>(SPREADING_RATE_LOWERBOUND);
    context_proxy->parameters[ContextBrokerProxy::INITIAL_SPREADING_RATE_UPPERBOUND] =
        static_cast<double>(SPREADING_RATE_UPPERBOUND);

    LOG_INFO("Creating AttentionBroker context tag=" + context_tag);
    bus->issue_bus_command(context_proxy);
    while (!context_proxy->is_context_created()) {
        Utils::sleep();
    }
    string context_key = context_proxy->get_key();
    LOG_INFO("Context created: " + context_key);
    return context_key;
}

int main(int argc, char* argv[]) {
    vector<string> positional;
    for (int i = 1; i < argc; ++i) {
        string arg(argv[i]);
        if (arg.rfind("--http-endpoint=", 0) == 0) {
            HTTP_ENDPOINT = arg.substr(string("--http-endpoint=").size());
        } else if (arg.rfind("--letter=", 0) == 0) {
            string letter = arg.substr(string("--letter=").size());
            if (letter.size() != 1) {
                cerr << "--letter= expects a single character, got: " << letter << endl;
                return 1;
            }
            LETTER_TO_COUNT = letter[0];
        } else if (arg.rfind("--", 0) == 0) {
            cerr << "Unknown flag: " << arg << endl;
            return 1;
        } else {
            positional.push_back(arg);
        }
    }

    // Required: client server ports config context_tag word
    // Optional trailing: RENT SPREAD_LO SPREAD_HI ELITISM SELECTION POP GENS
    if (positional.size() != 6 && positional.size() != 13) {
        cerr << "Usage: " << argv[0]
             << " <client_endpoint> <server_endpoint> <start_port:end_port> <config_file>"
                " <context_tag> <word>"
                " [RENT_RATE SPREAD_LO SPREAD_HI ELITISM_RATE SELECTION_RATE"
                " POPULATION_SIZE MAX_GENERATIONS]"
                " [--letter=c] [--http-endpoint=host:port]"
             << endl;
        cerr << endl;
        cerr << "Replicates notebooks/das-he-sentences-evolution.ipynb over CommandRouter HTTP:" << endl;
        cerr << "  query = (Contains $sentence1 (Word \"<word>\"))" << endl;
        cerr << "  fitness = local count_letter via remote_fitness_function WS" << endl;
        cerr << endl;
        cerr << "Notebook-like defaults: letter=c pop=50 gens=5 elitism=0.08 selection=0.10" << endl;
        return 1;
    }

    size_t cursor = 0;
    string client_endpoint = positional[cursor++];
    string server_endpoint = positional[cursor++];
    auto ports_range = Utils::parse_ports_range(positional[cursor++]);
    string config_file = positional[cursor++];
    string context_tag = positional[cursor++];
    string word_tag = positional[cursor++];

    if (positional.size() == 13) {
        RENT_RATE = Utils::string_to_float(positional[cursor++]);
        SPREADING_RATE_LOWERBOUND = Utils::string_to_float(positional[cursor++]);
        SPREADING_RATE_UPPERBOUND = Utils::string_to_float(positional[cursor++]);
        ELITISM_RATE = static_cast<double>(Utils::string_to_float(positional[cursor++]));
        SELECTION_RATE = static_cast<double>(Utils::string_to_float(positional[cursor++]));
        POPULATION_SIZE = static_cast<unsigned int>(Utils::string_to_int(positional[cursor++]));
        MAX_GENERATIONS = static_cast<unsigned int>(Utils::string_to_int(positional[cursor++]));
    }

    auto json_config = JsonConfigParser::load(config_file);
    auto atomdb_config = json_config.at_path("atomdb").get_or<JsonConfig>(JsonConfig());
    SystemParametersSingleton::init(json_config);
    AtomDBSingleton::init(atomdb_config);

    if (HTTP_ENDPOINT.empty()) {
        HTTP_ENDPOINT = json_config.at_path("agents.command_router.http_api.endpoint")
                            .get_or<string>("localhost:40009");
    }

    Utils::init_random(0);
    db = AtomDBSingleton::get_instance();
    FitnessFunctionRegistry::initialize_statics();
    CountLetterFunction::LETTER_TO_COUNT = LETTER_TO_COUNT;
    ServiceBusSingleton::init(client_endpoint, server_endpoint, ports_range.first, ports_range.second);
    auto bus = ServiceBusSingleton::get_instance();

    LOG_INFO("word=" + word_tag);
    LOG_INFO(string("letter=") + LETTER_TO_COUNT);
    LOG_INFO("HTTP_ENDPOINT=" + HTTP_ENDPOINT);
    LOG_INFO("POPULATION_SIZE=" + to_string(POPULATION_SIZE));
    LOG_INFO("MAX_GENERATIONS=" + to_string(MAX_GENERATIONS));
    LOG_INFO("ELITISM_RATE=" + to_string(ELITISM_RATE));
    LOG_INFO("SELECTION_RATE=" + to_string(SELECTION_RATE));

    string context_key = create_context(bus, context_tag);
    run_http_evolution(context_key, word_tag);
    return 0;
}
