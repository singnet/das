#include <signal.h>

#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "QueryEvolutionProxy.h"
#include "QueryAnswer.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"

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

string handle_to_atom(const char* handle) {
    shared_ptr<AtomDB> db = AtomDBSingleton::get_instance();
    shared_ptr<atomdb_api_types::AtomDocument> document = db->get_atom_document(handle);
    shared_ptr<atomdb_api_types::HandleList> targets = db->query_for_targets((char*) handle);
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

void run(const string& context, const string& word_tag1, const string& word_tag2) {
    string server_id = "0.0.0.0:24001";
    string client_id = "0.0.0.0:34001";

    AtomDBSingleton::init();
    shared_ptr<AtomDB> db = AtomDBSingleton::get_instance();
    ServiceBusSingleton::init(client_id, server_id, 63000, 63999);

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
    string word1 = "word1";
    string word2 = "word2";

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
                    node, symbol, "\"" + word_tag2 + "\"",
    };
    // clang-format on

    shared_ptr<QueryEvolutionProxy> proxy =
        make_shared<QueryEvolutionProxy>(or_two_words, "unit_test", context);
    proxy->set_population_size(100);
    service_bus->issue_bus_command(proxy);

    shared_ptr<QueryAnswer> query_answer;
    unsigned int count = 0;

    shared_ptr<atomdb_api_types::AtomDocument> sentence_document;
    shared_ptr<atomdb_api_types::AtomDocument> sentence_name_document;
    vector<string> sentences;
    while (!proxy->finished()) {
        if ((query_answer = proxy->pop()) == NULL) {
            Utils::sleep();
        } else {
            // cout << "------------------------------------------" << endl;
            // cout << query_answer->to_string() << endl;
            const char* handle;
            handle = query_answer->assignment.get(sentence1.c_str());
            // cout << string(handle) << endl;
            // cout << handle_to_atom(handle) << endl;
            sentence_document = db->get_atom_document(handle);
            handle = sentence_document->get("targets", 1);
            // cout << string(handle) << endl;
            // cout << handle_to_atom(handle) << endl;
            sentence_name_document = db->get_atom_document(handle);
            // cout << string(sentence_name_document->get("name")) << endl;
            set<string> to_highlight = {word_tag1, word_tag2};
            string sentence_name = string(sentence_name_document->get("name"));
            string highlighted_sentence_name = highlight(sentence_name, to_highlight);
            cout << highlighted_sentence_name << endl;
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
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <context> <word tag 1> <word tag 2>" << endl;
        exit(1);
    }
    string context = argv[1];
    string word_tag1 = argv[2];
    string word_tag2 = argv[3];

    run(context, word_tag1, word_tag2);
    return 0;
}
