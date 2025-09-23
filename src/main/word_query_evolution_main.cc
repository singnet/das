#include <signal.h>

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

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

#define MAX_QUERY_ANSWERS ((unsigned int) 100000)

using namespace std;
using namespace atomdb;
using namespace atom_space;
using namespace query_engine;
using namespace evolution;
using namespace service_bus;
using namespace attention_broker;

std::vector<std::string> split(string s, string delimiter) {
    std::vector<std::string> tokens;
    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        token = s.substr(0, pos);
        tokens.push_back(token);
        s.erase(0, pos + delimiter.length());
    }
    tokens.push_back(s);

    return tokens;
}

string highlight(const string& s, const set<string>& highlighted) {
    vector<string> tokens = split(s.substr(1, s.size() - 2), " ");
    string answer = "";
    for (unsigned int i = 0; i < tokens.size(); i++) {
        if (highlighted.find(tokens[i]) != highlighted.end()) {
            //"\033[31;1;4mHello\033[0m"
            answer += "\033[1;4m" + tokens[i] + "\033[0m";
        } else {
            answer += tokens[i];
        }
        if (i != (tokens.size() - 1)) {
            answer += " ";
        }
    }
    return answer;
}

class RemoteFitnessFunction : public FitnessFunction {
   public:
    shared_ptr<AtomDB> db;
    RemoteFitnessFunction() { this->db = AtomDBSingleton::get_instance(); }
    float eval(shared_ptr<QueryAnswer> answer) override {
        string variable_name = "sentence1";
        LOG_DEBUG(variable_name + ": " +
                  answer->metta_expression[answer->assignment.get(variable_name)]);
        return 1;
    }
};

string handle_to_atom(const string& handle) {
    shared_ptr<AtomDB> db = AtomDBSingleton::get_instance();
    shared_ptr<atomdb_api_types::AtomDocument> document = db->get_atom_document(handle);
    shared_ptr<atomdb_api_types::HandleList> targets = db->query_for_targets(handle);
    string answer;

    if (targets != NULL) {
        // is link
        answer += "<";
        answer += document->get("named_type");
        answer += ": [";
        for (unsigned int i = 0; i < targets->size(); i++) {
            answer += handle_to_atom(targets->get_handle(i));
            if (i < (targets->size() - 1)) {
                answer += ", ";
            }
        }
        answer += ">";
    } else {
        // is node
        answer += "(";
        answer += document->get("named_type");
        answer += ": ";
        answer += document->get("name");
        answer += ")";
    }

    return answer;
}

void run(const string& client_id,
         const string& server_id,
         size_t start_port,
         size_t end_port,
         const string& context_tag,
         const string& word_tag1,
         const string& word_tag2,
         double ELITISM_RATE) {
    AtomDBSingleton::init();
    shared_ptr<AtomDB> db = AtomDBSingleton::get_instance();
    ServiceBusSingleton::init(client_id, server_id, start_port, end_port);
    FitnessFunctionRegistry::initialize_statics();

    shared_ptr<ServiceBus> service_bus = ServiceBusSingleton::get_instance();

    bool USE_METTA_QUERY = true;

    // Symbols
    string and_operator = "AND";
    string or_operator = "OR";
    string link_template = "LINK_TEMPLATE";
    string link = "LINK";
    string node = "NODE";
    string variable = "VARIABLE";
    string expression = "Expression";
    string symbol = "Symbol";
    string sentence = "Sentence";
    string word = "Word";
    string contains = "Contains";

    // Variables
    string sentence1 = "sentence1";
    string sentence2 = "sentence2";
    string sentence3 = "sentence3";
    string word1 = "word1";

    // clang-format off
    vector<string> or_two_words = {
        or_operator, "2",
            link_template, expression, "3",
                node, symbol, contains,
                variable, sentence1,
                link, expression, "2",
                    node, symbol, word,
                    node, symbol, "\"" + word_tag1 + "\"",
            link_template, expression, "3",
                node, symbol, contains,
                variable, sentence1,
                link, expression, "2",
                    node, symbol, word,
                    node, symbol, "\"" + word_tag2 + "\""
    };
    string or_two_words_metta = "(or (Contains $sentence1 (Word \"" + word_tag1 + "\")) (Contains $sentence1 (Word \"" + word_tag2 + "\")))";

    // (Contains (Sentence "ede ebe cbe dca cbd fae bbb fce add eae") (Word "bbb"))
    // (Contains (Sentence "???") (Word "bbb"))
    // (Contains (Sentence "ede ebe cbe dca cbd fae bbb fce add eae") (Word "???"))
    // (Contains (Sentence "???") (Word "???"))

    vector<string> activation_spreading1 = {
        and_operator, "2",
            link_template, expression, "3",
                node, symbol, contains,
                variable, sentence1,
                variable, word1,
            link_template, expression, "3",
                node, symbol, contains,
                variable, sentence2,
                variable, word1,
    };
    string activation_spreading1_metta = "(and (Contains $sentence2 $word1) (Contains $sentence1 $word1))";

    vector<string> activation_spreading2 = {
        and_operator, "3",
            link_template, expression, "3",
                node, symbol, contains,
                variable, sentence1,
                variable, word1,
            link_template, expression, "3",
                node, symbol, contains,
                variable, sentence2,
                variable, word1,
            or_operator, "2",
                link_template, expression, "3",
                    node, symbol, contains,
                    variable, sentence2,
                    link, expression, "2",
                        node, symbol, word,
                        node, symbol, "\"" + word_tag1 + "\"",
                link_template, expression, "3",
                    node, symbol, contains,
                    variable, sentence2,
                    link, expression, "2",
                        node, symbol, word,
                        node, symbol, "\"" + word_tag2 + "\"",
    };

    vector<string> activation_spreading3 = {
        link_template, expression, "3",
            node, symbol, contains,
            variable, sentence1,
            variable, word1,
    };
    string activation_spreading3_metta = "(Contains $sentence1 $word1)";

    vector<string> context1 = {
        link_template, expression, "3",
            node, symbol, contains,
            variable, sentence1,
            variable, word1,
    };

    vector<string> context2 = {
        and_operator, "3",
            link_template, expression, "3",
                node, symbol, contains,
                variable, sentence1,
                variable, word1,
            link_template, expression, "3",
                node, symbol, contains,
                variable, sentence2,
                variable, word1,
            or_operator, "2",
                link_template, expression, "3",
                    node, symbol, contains,
                    variable, sentence2,
                    link, expression, "2",
                        node, symbol, word,
                        node, symbol, "\"" + word_tag1 + "\"",
                link_template, expression, "3",
                    node, symbol, contains,
                    variable, sentence2,
                    link, expression, "2",
                        node, symbol, word,
                        node, symbol, "\"" + word_tag2 + "\"",
    };
    // clang-format on

    // ---------------------------------------------------------------------------------------------
    // Query evolution request

    LOG_INFO("Setting up context");
    AtomSpace atom_space;
    // LinkSchema context_key(context1);
    // auto context = atom_space.create_context(context_tag, context_key);
    QueryAnswerElement sentence_link(sentence1);
    QueryAnswerElement word_link(word1);
    QueryAnswerElement contains_link(0);
    auto context = atom_space.create_context(
        context_tag, context1, {{contains_link, sentence_link}, {sentence_link, word_link}}, {});
    string context_str = context->get_key();
    LOG_INFO("Context " + context_str + " is ready");

    QueryEvolutionProxy* proxy_ptr;
    if (USE_METTA_QUERY) {
        proxy_ptr = new QueryEvolutionProxy({or_two_words_metta},
                                            {{activation_spreading3_metta}},
                                            {{{sentence1, sentence_link}}},
                                            {{sentence1, word_link}},
                                            context_str,
                                            "count_letter");
    } else {
        proxy_ptr = new QueryEvolutionProxy(or_two_words,
                                            {activation_spreading3},
                                            {{{sentence1, sentence_link}}},
                                            {{sentence1, word_link}},
                                            context_str,
                                            "count_letter");
    }

    shared_ptr<QueryEvolutionProxy> proxy(proxy_ptr);
    proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] = USE_METTA_QUERY;
    proxy->parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] = USE_METTA_QUERY;
    proxy->parameters[QueryEvolutionProxy::POPULATION_SIZE] = (unsigned int) 50;
    proxy->parameters[QueryEvolutionProxy::MAX_GENERATIONS] = (unsigned int) 20;
    proxy->parameters[QueryEvolutionProxy::ELITISM_RATE] = (double) ELITISM_RATE;
    proxy->parameters[QueryEvolutionProxy::SELECTION_RATE] = (double) 0.10;
    proxy->parameters[QueryEvolutionProxy::TOTAL_ATTENTION_TOKENS] = (unsigned int) 100000;
    proxy->parameters[BaseQueryProxy::MAX_BUNDLE_SIZE] = (unsigned int) 10000;
    service_bus->issue_bus_command(proxy);

    // ---------------------------------------------------------------------------------------------

    shared_ptr<QueryAnswer> query_answer;
    unsigned int count = 0;

    shared_ptr<atomdb_api_types::AtomDocument> sentence_document;
    shared_ptr<atomdb_api_types::AtomDocument> sentence_name_document;
    vector<string> sentences;
    while (!proxy->finished()) {
        if ((query_answer = proxy->pop()) == NULL) {
            Utils::sleep();
        } else {
            string handle = query_answer->assignment.get(sentence1.c_str());
            float fitness = query_answer->strength;
            sentence_document = db->get_atom_document(handle);
            handle = string(sentence_document->get("targets", 1));
            sentence_name_document = db->get_atom_document(handle);
            set<string> to_highlight = {word_tag1, word_tag2};
            string sentence_name = string(sentence_name_document->get("name"));
            string highlighted_sentence_name = highlight(sentence_name, to_highlight);
            cout << std::fixed << std::setw(6) << std::setprecision(4) << std::setfill('0') << fitness
                 << ": " << highlighted_sentence_name << endl;
            if (++count == MAX_QUERY_ANSWERS) {
                break;
            }
        }
    }
    if (count == 0) {
        cout << "No match for query" << endl;
        exit(0);
    } else {
        cout << "Count: " << count << endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 7) {
        cerr << "Usage: " << argv[0]
             << "    <client id> <server id> <start_port:end_port> <context_tag> <word tag 1> <word tag "
                "2> [RENT_RATE] [SPREADING_RATE_LOWERBOUND] [SPREADING_RATE_UPPERBOUND]"
             << endl;
        exit(1);
    }
    float RENT_RATE = 0.25;
    float SPREADING_RATE_LOWERBOUND = 0.50;
    float SPREADING_RATE_UPPERBOUND = 0.70;
    double ELITISM_RATE = 0.08;
    string client_id = argv[1];
    string server_id = argv[2];
    auto ports_range = Utils::parse_ports_range(argv[3]);
    string context_tag = argv[4];
    string word_tag1 = argv[5];
    string word_tag2 = argv[6];
    if (argc > 7) {
        RENT_RATE = Utils::string_to_float(string(argv[7]));
        SPREADING_RATE_LOWERBOUND = Utils::string_to_float(string(argv[8]));
        SPREADING_RATE_UPPERBOUND = Utils::string_to_float(string(argv[9]));
        ELITISM_RATE = (double) Utils::string_to_float(string(argv[10]));
    }
    AttentionBrokerClient::set_parameters(
        RENT_RATE, SPREADING_RATE_LOWERBOUND, SPREADING_RATE_UPPERBOUND);
    LOG_INFO("ELITISM_RATE: " << ELITISM_RATE);
    run(client_id,
        server_id,
        ports_range.first,
        ports_range.second,
        context_tag,
        word_tag1,
        word_tag2,
        ELITISM_RATE);
    return 0;
}
