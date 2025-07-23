#include <signal.h>

#include <iomanip>
#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "FitnessFunctionRegistry.h"
#include "QueryAnswer.h"
#include "QueryEvolutionProxy.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"
#include "CountLetterFunction.h"

#define MAX_QUERY_ANSWERS ((unsigned int) 100000)

using namespace std;
using namespace atomdb;
using namespace query_engine;
using namespace evolution;
using namespace service_bus;

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
         const string& context,
         const string& word_tag1,
         const string& word_tag2) {
    AtomDBSingleton::init();
    shared_ptr<AtomDB> db = AtomDBSingleton::get_instance();
    ServiceBusSingleton::init(client_id, server_id, start_port, end_port);
    FitnessFunctionRegistry::initialize_statics();

    shared_ptr<ServiceBus> service_bus = ServiceBusSingleton::get_instance();

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
    vector<string> activation_spreading = {
        and_operator, "2",
            link_template, expression, "3",
                node, symbol, contains,
                variable, sentence1,
                variable, word1,
            link_template, expression, "3",
                node, symbol, contains,
                variable, sentence3,
                variable, word1,
    };
    // clang-format on

    // ---------------------------------------------------------------------------------------------
    // Query evolution request

    QueryEvolutionProxy* proxy_ptr = new QueryEvolutionProxy(
        or_two_words, activation_spreading, {sentence3}, context, "count_letter");
    shared_ptr<QueryEvolutionProxy> proxy(proxy_ptr);
    proxy->parameters[QueryEvolutionProxy::POPULATION_SIZE] = (unsigned int) 100;
    proxy->parameters[QueryEvolutionProxy::MAX_GENERATIONS] = (unsigned int) 10;
    proxy->parameters[QueryEvolutionProxy::ELITISM_RATE] = (double) 0.01;
    proxy->parameters[QueryEvolutionProxy::SELECTION_RATE] = (double) 0.02;
    proxy->parameters[BaseQueryProxy::MAX_BUNDLE_SIZE] = (unsigned int) 1;
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
    if (argc != 7) {
        cerr << "Usage: " << argv[0]
             << "    <client id> <server id> <start_port:end_port> <context> <word tag 1> <word tag 2>"
             << endl;
        exit(1);
    }
    string client_id = argv[1];
    string server_id = argv[2];
    auto ports_range = Utils::parse_ports_range(argv[3]);
    string context = argv[4];
    string word_tag1 = argv[5];
    string word_tag2 = argv[6];

    run(client_id, server_id, ports_range.first, ports_range.second, context, word_tag1, word_tag2);
    return 0;
}
