#include <iostream>
#include <string>
#include <stack>

#include <signal.h>

#include "DASNode.h"
#include "RemoteIterator.h"
#include "QueryAnswer.h"
#include "AtomDBSingleton.h"
#include "AtomDB.h"
#include "Utils.h"

#define MAX_QUERY_ANSWERS ((unsigned int) 1000)

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

double compute_sim1(const vector<string> &tokens1, const vector<string> &tokens2) {

    unsigned int count = 0;

    /*
    for (unsigned int i = 0; i < tokens1.size(); i++) {
        for (unsigned int j = 0; j < tokens2.size(); j++) {
            count++;
            break;
        }
    }

    for (unsigned int i = 0; i < tokens2.size(); i++) {
        for (unsigned int j = 0; j < tokens2.size(); j++) {
            count++;
            break;
        }
    }
    */

    for (auto token1: tokens1) {
        for (auto token2: tokens2) {
            if (token1 == token2) {
                count++;
                break;
            }
        }
    }

    for (auto token2: tokens2) {
        for (auto token1: tokens1) {
            if (token2 == token1) {
                count++;
                break;
            }
        }
    }

    return ((1.0) * count) / (tokens1.size() + tokens2.size());
}

double compute_sim2(const vector<string> &tokens1, const vector<string> &tokens2) {

    if (tokens1.size() != tokens2.size()) {
        return 0.0;
    }
    unsigned int count = 0;
    unsigned int total_length = 0;
    for (unsigned int i = 0; i < tokens1.size(); i++) {
        for (unsigned int j = 0; j < tokens1[i].length(); j++) {
            if (tokens1[i][j] == tokens2[i][j]) {
                count++;
            }
        }
        total_length += tokens1[i].length();
    }
    return (1.0 * count) / total_length;
}

string highlight(const vector<string> &tokens1, const vector<string> &tokens2, const set<string> &highlighted) {
    //printf("\033[31;1;4mHello\033[0m");
    string answer = "";
    bool token_flag, char_flag, word_flag;
    for (unsigned int i = 0; i < tokens1.size(); i++) {
        token_flag = (highlighted.find(tokens1[i]) != highlighted.end());
        word_flag = false;
        if (highlighted.size() == 0) {
            for (auto token: tokens2) {
                if (tokens1[i] == token) {
                    word_flag = true;
                    break;
                }
            }
        }
        for (unsigned int j = 0; j < tokens1[i].length(); j++) {
            if (tokens1.size() == tokens2.size()) {
                char_flag = (tokens1[i][j] == tokens2[i][j]);
            } else {
                char_flag = false;
            }
            char_flag = false; // XXXXX
            if (token_flag || char_flag || word_flag) {
                answer += "\033[";
                if (token_flag) {
                    answer += "1;4";
                    if (char_flag || word_flag) {
                        answer += ";";
                    }
                }
                if (char_flag) {
                    answer += "4";
                    if (word_flag) {
                        answer += ";";
                    }
                }
                if (word_flag) {
                    answer += "7";
                }
                answer += "m";
                answer += tokens1[i][j];
                answer += "\033[0m";
            } else {
                answer += tokens1[i][j];
            }
        }
        if (i != (tokens1.size() - 1)) {
            answer += " ";
        }
    }
    return answer;
}

void build_link(const string &link_type_tag, const string str1, const string str2, double threshold, stack<string> &output, const set<string> &highlighted) {

    string sentence1 = str1.substr(1, str1.size() - 2);
    string sentence2 = str2.substr(1, str2.size() - 2);

    vector<string> tokens1 = split(sentence1, " ");
    vector<string> tokens2 = split(sentence2, " ");

    double v1 = compute_sim1(tokens1, tokens2);
    //double v2 = compute_sim2(tokens1, tokens2);

    if (v1 >= threshold) {
        //output.push(std::to_string(v1) + ": " + highlight(tokens1, tokens2, highlighted));
        //output.push(std::to_string(v2) + ": " + highlight(tokens2, tokens1, highlighted));
        output.push(highlight(tokens1, tokens2, highlighted));
        output.push(highlight(tokens2, tokens1, highlighted));
    }
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
    const string &link_type_tag,
    const set<string> highlighted) {

    string server_id = "localhost:31700";
    string client_id = "localhost:31701";

    AtomDBSingleton::init();
    shared_ptr<AtomDB> db = AtomDBSingleton::get_instance();

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
    string similarity = "Similarity";
    string contains = "Contains";
    string sentence1 = "sentence1";
    string sentence2 = "sentence2";
    string word1 = "word1";
    string word2 = "word2";
    string tv1 = "tv1";

    // (Contains (Sentence "aef cbe dfb fbe eca eff bad") (Word "eff"))

    vector<string> query_same_word = {
        and_operator, "2",
            link_template, expression, "3",
                node, symbol, contains,
                variable, sentence1,
                variable, word1,
            link_template, expression, "3", 
                node, symbol, contains,
                variable, sentence2,
                variable, word1
    };

    vector<string> query_same_size {
        or_operator, "1",
        link_template, expression, "4",
            node, symbol, similarity,
            variable, sentence1,
            variable, sentence2,
            variable, tv1
    };

    DASNode client(client_id, server_id);
    QueryAnswer *query_answer;
    unsigned int count = 0;
    RemoteIterator *response;

    if (link_type_tag == "LINK1") {
        response = client.pattern_matcher_query(query_same_word, context);
    } else if (link_type_tag == "LINK2") {
        response = client.pattern_matcher_query(query_same_size, context, true);
    } else {
        Utils::error("Invalid link_type_tag: " + link_type_tag);
    }

    shared_ptr<atomdb_api_types::AtomDocument> sentence_document1;
    shared_ptr<atomdb_api_types::AtomDocument> sentence_document2;
    shared_ptr<atomdb_api_types::AtomDocument> sentence_symbol_document1;
    shared_ptr<atomdb_api_types::AtomDocument> sentence_symbol_document2;
    stack<string> output;
    set<string> already_inserted_links;
    while (! response->finished()) {
        if ((query_answer = response->pop()) == NULL) {
            Utils::sleep();
        } else {
            if (! strcmp(query_answer->assignment.get(sentence1.c_str()), query_answer->assignment.get(sentence2.c_str()))) {
                continue;
            }
            //cout << query_answer->to_string() << endl;
            //cout << handle_to_atom(query_answer->handles[0]) << endl;
            //cout << handle_to_atom(query_answer->handles[1]) << endl;
            sentence_document1 = db->get_atom_document(query_answer->assignment.get(sentence1.c_str()));
            sentence_document2 = db->get_atom_document(query_answer->assignment.get(sentence2.c_str()));
            sentence_symbol_document1 = db->get_atom_document(sentence_document1->get("targets", 1));
            sentence_symbol_document2 = db->get_atom_document(sentence_document2->get("targets", 1));
            string s1 = string(sentence_symbol_document1->get("name"));
            string s2 = string(sentence_symbol_document2->get("name"));
            if ((already_inserted_links.find(s1 + s2) == already_inserted_links.end()) && 
                (already_inserted_links.find(s2 + s1) == already_inserted_links.end())) {
                build_link(link_type_tag, s1, s2, 0.0, output, highlighted);
                already_inserted_links.insert(s1 + s2);
                already_inserted_links.insert(s2 + s1);
            }

            if (++count == MAX_QUERY_ANSWERS) {
                break;
            }
        }
    }
    if (count == 0) {
        cout << "No match for query" << endl;
    } else {
        while (! output.empty()) {
            cout << output.top() << endl;
            output.pop();
            cout << output.top() << endl;
            output.pop();
            cout << endl;
        }
    }

    delete response;
}

int main(int argc, char* argv[]) {

    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <context> <link type tag> <word>*" << endl;
        exit(1);
    }
    signal(SIGINT, &ctrl_c_handler);
    string context = argv[1];
    string link_type_tag = argv[2];

    if ((link_type_tag != "LINK1") && (link_type_tag != "LINK2")) {
        Utils::error("Invalid link_type_tag: " + link_type_tag);
    }

    set<string> highlighted;
    for (int i = 3; i < argc; i++) {
        highlighted.insert(string(argv[i]));
    }

    run(context, link_type_tag, highlighted);
    return 0;
}
