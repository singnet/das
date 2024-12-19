#include <iostream>
#include <string>

#include <signal.h>

#include "DASNode.h"
#include "RemoteIterator.h"
#include "QueryAnswer.h"
#include "AtomDBSingleton.h"
#include "AtomDB.h"
#include "Utils.h"

#define MAX_QUERY_ANSWERS ((unsigned int) 500)

using namespace std;

void ctrl_c_handler(int) {
    std::cout << "Stopping link creation engine server..." << std::endl;
    std::cout << "Done." << std::endl;
    exit(0);
}

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

string highlight(const string &s, const set<string> &highlighted) {
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



string handle_to_atom(const char *handle) {

    shared_ptr<AtomDB> db = AtomDBSingleton::get_instance();
    shared_ptr<atomdb_api_types::AtomDocument> document = db->get_atom_document(handle);
    shared_ptr<atomdb_api_types::HandleList> targets = db->query_for_targets((char *) handle);
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

void run(
    const string &context,
    const string &word_tag) {

    string server_id = "localhost:31700";
    string client_id = "localhost:31701";

    AtomDBSingleton::init();
    shared_ptr<AtomDB> db = AtomDBSingleton::get_instance();

    string and_operator = "AND";
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

    vector<string> query_word = {
        link_template, expression, "3", 
            node, symbol, contains, 
            variable, sentence1, 
            link, expression, "2", 
                node, symbol, word, 
                node, symbol, "\"" + word_tag + "\""
    };

    DASNode client(client_id, server_id);
    QueryAnswer *query_answer;
    unsigned int count = 0;
    RemoteIterator *response = client.pattern_matcher_query(query_word, context, true);
    shared_ptr<atomdb_api_types::AtomDocument> sentence_document;
    shared_ptr<atomdb_api_types::AtomDocument> sentence_name_document;
    vector<string> sentences;
    while (! response->finished()) {
        if ((query_answer = response->pop()) == NULL) {
            Utils::sleep();
        } else {
            //cout << "------------------------------------------" << endl;
            //cout << query_answer->to_string() << endl;
            const char *handle;
            handle = query_answer->assignment.get(sentence1.c_str());
            //cout << string(handle) << endl;
            //cout << handle_to_atom(handle) << endl;
            sentence_document = db->get_atom_document(handle);
            handle = sentence_document->get("targets", 1);
            //cout << string(handle) << endl;
            //cout << handle_to_atom(handle) << endl;
            sentence_name_document = db->get_atom_document(handle);
            // cout << string(sentence_name_document->get("name")) << endl;
            set<string> to_highlight;
            to_highlight.insert(word_tag);
            string sentence_name = string(sentence_name_document->get("name"));
            string highlighted_sentence_name = highlight(sentence_name, to_highlight);
            string w = "\"" + word_tag + "\"";
            string line = "(Contains (Sentence " +
                          highlighted_sentence_name +
                          ") (Word \"" +
                          highlight(w, to_highlight) +
                          "\"))";
            cout << line << endl;
            if (++count == MAX_QUERY_ANSWERS) {
                break;
            }
        }
    }
    if (count == 0) {
        cout << "No match for query" << endl;
        exit(0);
    }

    delete response;
}

int main(int argc, char* argv[]) {

    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <context> <word tag>" << endl;
        exit(1);
    }
    signal(SIGINT, &ctrl_c_handler);
    string context = argv[1];
    string word_tag = argv[2];

    run(context, word_tag);
    return 0;
}
