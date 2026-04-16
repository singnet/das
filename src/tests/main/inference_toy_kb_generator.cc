#include "Logger.h"

#include <vector>
#include <string>
#include <iostream>
#include <fstream>

using namespace commons;

static string METTA_FILE_PREFIX = "./inference_toy";

static unsigned int KB_SIZE = 0;
static unsigned int CONCEPT_SIZE = 0;
static unsigned int CONCEPT_PART_SIZE = 0;
static string CHARSET = "";
static unsigned int BCD_SHORT_COUNT = 0;

static string EVALUATION = "Evaluation";
static string PREDICATE = "Predicate";
static string CONCEPT = "Concept";

// --------------------------------------------------------------------------------
// Utils

string concept_name(vector<string>& _concept) {
    string name = "\"";
    for (auto& concept_part : _concept) {
        name += concept_part;
        name += " ";
    }
    name.pop_back();
    name += "\"";
    return name;
}

string predicate_name(string base, string detail) {
    string name = "\"" + base;
    if (detail != "") {
        name += "_" + detail;
    }
    name += "\"";
    return name;
}

void print_concepts(vector<vector<string>>& concepts) {
    string message = "Concepts:\n";
    for (auto& _concept : concepts) {
        message += concept_name(_concept);
        message += "\n";
    }
    message.pop_back();
    LOG_INFO(message);
}

void print_setup() {
    LOG_INFO("KB_SIZE: " + std::to_string(KB_SIZE));
    LOG_INFO("CONCEPT_SIZE: " + std::to_string(CONCEPT_SIZE));
    LOG_INFO("CONCEPT_PART_SIZE: " + std::to_string(CONCEPT_PART_SIZE));
    LOG_INFO("CHARSET: " + CHARSET);
}

void create_concepts(vector<vector<string>>& concepts) {
    for (unsigned int i = 0; i < KB_SIZE; i++) {
        concepts.push_back({});
        for (unsigned int j = 0; j < CONCEPT_SIZE; j++) {
            concepts[i].push_back(Utils::random_string(CONCEPT_PART_SIZE, CHARSET));
        }
    }
}

// --------------------------------------------------------------------------------
// Hidden predicates

bool predicate_bcd(vector<string>& _concept) {
    bool flag_b = false;
    bool flag_c = false;
    bool flag_d = false;
    for (string& part : _concept) {
        char k = ' ';
        unsigned int count = 0;
        for (char c : part) {
            if (c == ' ') {
                Utils::error("Invalid char ' ' in context part");
            }
            if (c == k) {
                count++;
            } else {
                if (k != ' ') break;
                k = c;
                count = 1;
            }
        }
        if (count == CONCEPT_PART_SIZE) {
            if (k == 'b') flag_b = true;
            if (k == 'c') flag_c = true;
            if (k == 'd') flag_d = true;
            if (flag_b && flag_c && flag_d) return true;
        }
    }
    return false;
}

bool hidden_predicates(vector<string>& _concept); // Just the header to avoid compilation error

void enforce_bcd_sort(vector<vector<string>> &concepts) {
    unsigned int count = 0;
    for (auto& _concept : concepts) {
        if (predicate_bcd(_concept)) {
            sort(_concept.begin(), _concept.end());
            count++;
        }
    }
    unsigned int cursor = 0;
    while (count < BCD_SHORT_COUNT) {
        if (cursor == concepts.size()) {
            Utils::error("Invalid parameters. BCD_SHORT_COUNT = " + std::to_string(BCD_SHORT_COUNT) + " concepts.size() = " + std::to_string(concepts.size()));
            return;
        }
        if (! hidden_predicates(concepts[cursor])) {
            concepts[cursor][0] = Utils::random_string(CONCEPT_PART_SIZE, "b");
            concepts[cursor][1] = Utils::random_string(CONCEPT_PART_SIZE, "c");
            concepts[cursor][2] = Utils::random_string(CONCEPT_PART_SIZE, "d");
            sort(concepts[cursor].begin(), concepts[cursor].end());
            count++;
        }
        cursor++;
    }
}

bool hidden_predicates(vector<string>& _concept) {
    return predicate_bcd(_concept);
}

void enforce_hidden_predicates(vector<vector<string>> &concepts) {
    enforce_bcd_sort(concepts);
}

// --------------------------------------------------------------------------------
// Export to metta

void export_contains(ofstream& metta_file, vector<string> &_concept) {
    for (string& part : _concept) {
        string predicate_exp = "(" + PREDICATE + " " + predicate_name("contains", part) + ")";
        string concept_exp = "(" + CONCEPT + " " + concept_name(_concept) + ")";
        string evaluation_exp = "(" + EVALUATION + " " + predicate_exp + " " + concept_exp + ")";
        metta_file << evaluation_exp << endl;
    }
}

void export_start_end(ofstream& metta_file, vector<string> &_concept, bool start_flag) {
    string tag = (start_flag ? "starts_with" : "ends_with");
    string key = (start_flag ? _concept[0] : _concept[CONCEPT_SIZE - 1]);
    string predicate_exp = "(" + PREDICATE + " " + predicate_name(tag, key) + ")";
    string concept_exp = "(" + CONCEPT + " " + concept_name(_concept) + ")";
    string evaluation_exp = "(" + EVALUATION + " " + predicate_exp + " " + concept_exp + ")";
    metta_file << evaluation_exp << endl;
}

void export_concept(ofstream& metta_file, vector<string> &_concept) {
    export_contains(metta_file, _concept);
    export_start_end(metta_file, _concept, true);
    export_start_end(metta_file, _concept, false);
}

void export_metta(vector<vector<string>> &concepts) {
    string file_name = METTA_FILE_PREFIX + "_" + std::to_string(KB_SIZE) + "_" + std::to_string(CONCEPT_SIZE) + "_" + std::to_string(CONCEPT_PART_SIZE) + "_" + CHARSET + "_" + std::to_string(BCD_SHORT_COUNT) + ".metta";
    ofstream metta_file(file_name);
    if (! metta_file.is_open()) {
        Utils::error("Unable to open file: " + file_name);
    }
    for (auto& _concept : concepts) {
        export_concept(metta_file, _concept);
    }
    metta_file.close();
}

// --------------------------------------------------------------------------------
// Main

int main(int argc, char* argv[]) {

    if (argc != 6) {
        Utils::error("Usage: inference_toy_kb_generator <KB size> <concept size> <concept part size> <charset> <bcd_short count>");
    }

    unsigned int count = 1;
    KB_SIZE = Utils::string_to_uint(string(argv[count++]));
    CONCEPT_SIZE = Utils::string_to_uint(string(argv[count++]));
    CONCEPT_PART_SIZE = Utils::string_to_uint(string(argv[count++]));
    CHARSET = string(argv[count++]);
    BCD_SHORT_COUNT = Utils::string_to_uint(string(argv[count++]));
    print_setup();

    vector<vector<string>> concepts;
    LOG_INFO("Creating initial concepts...");
    create_concepts(concepts);
    LOG_INFO("Enforcing BCD-Sort predicate...");
    enforce_hidden_predicates(concepts);
    //print_concepts(concepts);
    LOG_INFO("Exporting MeTTa file...");
    export_metta(concepts);
    LOG_INFO("Finished");
}
