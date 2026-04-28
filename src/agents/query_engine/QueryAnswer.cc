#include "QueryAnswer.h"

#include <cmath>
#include <cstring>
#include <iostream>

#include "LinkSchema.h"
#include "Utils.h"

using namespace query_engine;
using namespace commons;
using namespace atoms;

QueryAnswer::QueryAnswer() : QueryAnswer(0.0) {}

QueryAnswer::QueryAnswer(double importance) {
    this->importance = importance;
    this->strength = 0;
    this->handles.push_back({});
}

QueryAnswer::QueryAnswer(const string& handle, double importance) {
    this->importance = importance;
    this->handles.push_back({});
    this->handles[0].push_back(handle);
    this->strength = 0;
}

QueryAnswer::~QueryAnswer() {}

QueryAnswer* QueryAnswer::copy(QueryAnswer* other) {  // Static method
    QueryAnswer* copy = new QueryAnswer(other->importance);
    copy->strength = other->strength;
    copy->assignment = other->assignment;
    copy->handles = other->handles;
    copy->metta_expression = other->metta_expression;
    return copy;
}

void QueryAnswer::merge_paths(QueryAnswer* other) {
    unsigned int original_path_count = this->handles.size() - 1;
    unsigned int cursor_this = original_path_count + 1;
    for (unsigned int cursor_other = 1; cursor_other < other->handles.size(); cursor_other++) {
        bool already_exists = false;
        for (unsigned int i = 1; i <= original_path_count; i++) {
            if (other->handles[cursor_other] == this->handles[i]) {
                already_exists = true;
                break;
            }
        }
        if (!already_exists) {
            this->handles.push_back({});
            this->handles[cursor_this++] = other->handles[cursor_other];
        }
    }
}

bool QueryAnswer::merge(QueryAnswer* other, bool merge_handles) {
    if (this->assignment.is_compatible(other->assignment)) {
        for (auto pair : other->assignment.table) {
            if (!other->metta_expression[pair.second].empty()) {
                this->metta_expression[pair.second] = other->metta_expression[pair.second];
            }
        }
        this->assignment.add_assignments(other->assignment);
        if (merge_handles) {
            this->importance = fmax(this->importance, other->importance);
            this->strength = this->strength * other->strength;
            for (string handle1 : other->handles[0]) {
                bool flag = true;
                for (string handle2 : this->handles[0]) {
                    if (handle1 == handle2) {
                        flag = false;
                    }
                }
                if (flag) {
                    this->handles[0].push_back(handle1);
                }
                // Only merge if the other has a non-empty metta expression
                if (!other->metta_expression[handle1].empty()) {
                    this->metta_expression[handle1] = other->metta_expression[handle1];
                }
            }
            if (other->handles.size() > 1) {
                this->merge_paths(other);
            }
        }
        return true;
    } else {
        return false;
    }
}

string QueryAnswer::to_string(bool metta_flag) {
    string answer = "QueryAnswer<" + std::to_string(this->handles[0].size()) + ",";
    answer += std::to_string(this->assignment.variable_count()) + "> [";
    for (auto& vector : this->handles) {
        answer += "[";
        bool empty_flag = true;
        for (string handle : vector) {
            answer += (metta_flag ? metta_expression[handle] :  handle);
            answer += ", ";
            empty_flag = false;
        }
        if (!empty_flag) {
            answer.pop_back();
            answer.pop_back();
        }
        answer += "]";
    }
    answer += "] ";
    if (metta_flag) {
        bool empty_flag = true;
        answer += "{";
        for (auto pair : this->assignment.table) {
            answer += "(" + pair.first + ": " + metta_expression[pair.second] + "), ";
            empty_flag = false;
        }
        if (!empty_flag) {
            answer.pop_back();
            answer.pop_back();
        }
        answer += "}";
    } else {
        answer += this->assignment.to_string();
    }
    answer += " (" + std::to_string(this->strength) + ", " + std::to_string(this->importance) + ")";
    return answer;
}

const string& QueryAnswer::tokenize() {
    // char_count is computed to be slightly larger than actually required by assuming
    // e.g. 3 digits to represent sizes
    char strength_buffer[13];
    char importance_buffer[13];
    sprintf(strength_buffer, "%.10f", this->strength);
    sprintf(importance_buffer, "%.10f", this->importance);
    unsigned int char_count =
        13    // strength with 10 decimals + space
        + 13  // importance with 10 decimals + space
        + 4   // (up to 3 digits) to represent this->assignment.size + space
        + this->assignment.table.size() *
              (MAX_VARIABLE_NAME_SIZE + HANDLE_HASH_SIZE + 2)  // label<space>handle<space>
        + 4               // (up to 3 digits) to represent this->metta_expression.size + space
        + 4;              // (up to 3 digits) to represent this->handles.size() + space
    for (auto& vector : this->handles) {
        char_count += 4;  // (up to 3 digits) to represent this->handles[i].size() + space
        char_count += vector.size() * (HANDLE_HASH_SIZE + 1);  // handles[i] + spaces
    }

    this->token_representation.clear();
    this->token_representation.reserve(char_count);
    string space = " ";
    this->token_representation += strength_buffer;
    this->token_representation += space;
    this->token_representation += importance_buffer;
    this->token_representation += space;
    this->token_representation += std::to_string(this->handles.size());
    this->token_representation += space;
    for (auto& vector : this->handles) {
        this->token_representation += std::to_string(vector.size());
        this->token_representation += space;
        for (string handle : vector) {
            this->token_representation += handle;
            this->token_representation += space;
        }
    }
    this->token_representation += std::to_string(this->assignment.table.size());
    this->token_representation += space;
    for (auto pair : this->assignment.table) {
        this->token_representation += pair.first;
        this->token_representation += space;
        this->token_representation += pair.second;
        this->token_representation += space;
    }
    this->token_representation += std::to_string(this->metta_expression.size());
    this->token_representation += space;
    for (auto pair : this->metta_expression) {
        this->token_representation += pair.first;
        this->token_representation += space;
        this->token_representation += pair.second;
        this->token_representation += space;
    }

    return this->token_representation;
}

static inline void read_token(const char* token_string,
                              unsigned int& cursor,
                              char* token,
                              unsigned int token_size) {
    unsigned int cursor_token = 0;
    while (token_string[cursor] != ' ') {
        if ((cursor_token == token_size) || (token_string[cursor] == '\0')) {
            Utils::error("Invalid token string");
        }
        token[cursor_token++] = token_string[cursor++];
    }
    token[cursor_token] = '\0';
    cursor++;
}

static inline string read_metta_expression(const char* token_string, unsigned int& cursor) {
    unsigned int start = cursor;
    unsigned int unmatched = 1;
    char open_char;
    char close_char;
    if (token_string[start] == '(') {
        open_char = '(';
        close_char = ')';
    } else if (token_string[start] == '\"') {
        open_char = '\"';
        close_char = '\"';
    } else {
        open_char = ' ';
        close_char = ' ';
    }
    do {
        cursor++;
        if (token_string[cursor] == '\0') {
            Utils::error("Invalid metta expression string");
        } else if ((token_string[cursor] == close_char) && (token_string[cursor - 1] != '\\')) {
            unmatched--;
        } else if ((token_string[cursor] == open_char) && (token_string[cursor - 1] != '\\')) {
            unmatched++;
        }
    } while (unmatched > 0);
    if (close_char != ' ') {
        cursor++;
    }
    unsigned int end = cursor++;
    string answer(token_string + start, token_string + end);
    return answer;
}

void QueryAnswer::untokenize(const string& tokens) {
    const char* token_string = tokens.c_str();
    char number[4];
    char strength[13];
    char importance[13];
    char handle[HANDLE_HASH_SIZE];
    char label[MAX_VARIABLE_NAME_SIZE];

    unsigned int cursor = 0;

    read_token(token_string, cursor, strength, 13);
    this->strength = std::stod(strength);

    read_token(token_string, cursor, importance, 13);
    this->importance = std::stod(importance);

    read_token(token_string, cursor, number, 4);
    unsigned int handles_size = (unsigned int) std::stoi(number);

    if (handles_size >= MAX_NUMBER_OF_OPERATION_CLAUSES) {
        Utils::error("Invalid handles_size: " + std::to_string(handles_size) +
                     " untokenizing QueryAnswer");
    } else {
        for (unsigned int i = 0; i < handles_size; i++) {
            read_token(token_string, cursor, number, 4);
            unsigned int vector_size = (unsigned int) std::stoi(number);
            unsigned int path_index = 0;
            if (i > 0) {
                path_index = this->add_path();
            }
            for (unsigned int j = 0; j < vector_size; j++) {
                read_token(token_string, cursor, handle, HANDLE_HASH_SIZE);
                if (i == 0) {
                    // handles vector
                    this->add_handle(handle);
                } else {
                    // paths
                    this->add_path_element(path_index, handle);
                }
            }
        }
    }

    read_token(token_string, cursor, number, 4);
    unsigned int assignment_size = (unsigned int) std::stoi(number);

    if (assignment_size > MAX_NUMBER_OF_VARIABLES_IN_QUERY) {
        Utils::error("Invalid number of assignments: " + std::to_string(assignment_size) +
                     " untokenizing QueryAnswer");
    }

    for (unsigned int i = 0; i < assignment_size; i++) {
        read_token(token_string, cursor, label, MAX_VARIABLE_NAME_SIZE);
        read_token(token_string, cursor, handle, HANDLE_HASH_SIZE);
        this->assignment.assign(string(label), string(handle));
    }

    read_token(token_string, cursor, number, 4);
    unsigned int metta_mapping_size = (unsigned int) std::stoi(number);
    string metta_expression;
    if (metta_mapping_size > 0) {
        for (unsigned int i = 0; i < metta_mapping_size; i++) {
            read_token(token_string, cursor, handle, HANDLE_HASH_SIZE);
            metta_expression = read_metta_expression(token_string, cursor);
            this->metta_expression[string(handle)] = metta_expression;
        }
    }

    if (token_string[cursor] != '\0') {
        Utils::error("Invalid token string - invalid text after QueryAnswer definition");
    }
}

string QueryAnswer::get(const QueryAnswerElement& key, bool return_empty_when_not_found) {
    string answer = "";
    switch (key.type) {
        case QueryAnswerElement::HANDLE:
            return get(key.index, return_empty_when_not_found);
        case QueryAnswerElement::VARIABLE:
            return get(key.name, return_empty_when_not_found);
        default:
            Utils::error("Invalid QueryAnswerElemente: " + std::to_string(key.type));
    }
    return answer;
}

string QueryAnswer::get(const string& key, bool return_empty_when_not_found) {
    string answer = this->assignment.get(key);
    if ((answer == "") && (!return_empty_when_not_found)) {
        Utils::error("Invalid variable name: " + key);
    }
    return answer;
}

string QueryAnswer::get(unsigned int key, bool return_empty_when_not_found) {
    string answer = "";
    if (key < this->handles[0].size()) {
        answer = this->handles[0][key];
    } else {
        if (!return_empty_when_not_found) {
            Utils::error("Invalid handle index: " + std::to_string(key));
        }
    }
    return answer;
}

void QueryAnswer::rewrite_query(const vector<string>& original_query,
                                map<string, QueryAnswerElement>& replacements,
                                vector<string>& new_query) {
    new_query.clear();
    unsigned int cursor = 0;
    unsigned int size = original_query.size();
    while (cursor < size) {
        string token = original_query[cursor++];
        if (token == LinkSchema::UNTYPED_VARIABLE) {
            if (cursor == size) {
                Utils::error("Invalid query");
                return;
            }
            token = original_query[cursor++];
            auto iterator = replacements.find(token);
            if (iterator != replacements.end()) {
                string value = this->get(iterator->second);
                new_query.push_back(LinkSchema::ATOM);
                new_query.push_back(value);
            } else {
                new_query.push_back(LinkSchema::UNTYPED_VARIABLE);
                new_query.push_back(token);
            }
        } else {
            new_query.push_back(token);
        }
    }
}

void QueryAnswer::add_handle(const string& handle) { this->handles[0].push_back(handle); }

unsigned int QueryAnswer::add_path() {
    if (this->handles.size() == 0) {
        Utils::error("Invalid QueryAnswer setup. Trying to add a path to an uninitialized  QueryAnswer");
        return 0;
    } else {
        unsigned int new_index = this->handles.size() - 1;
        this->handles.push_back({});
        return new_index;
    }
}

void QueryAnswer::add_path_element(unsigned int path_index, const string& handle) {
    if (this->handles.size() == 0) {
        Utils::error(
            "Invalid QueryAnswer setup. Trying to add a path element to an uninitialized  QueryAnswer");
    } else {
        if (path_index >= (this->handles.size() - 1)) {
            Utils::error("Invalid path index: " + std::to_string(path_index) +
                         " QueryAnswer: " + to_string());
        } else {
            this->handles[path_index + 1].push_back(handle);
        }
    }
}

unsigned int QueryAnswer::get_handles_size() { return this->handles[0].size(); }

unsigned int QueryAnswer::get_paths_size() {
    unsigned int size = this->handles.size();
    if (size == 0) {
        return 0;
    } else {
        return size - 1;
    }
}

vector<string>& QueryAnswer::get_handles_vector() { return this->handles[0]; }

vector<string>& QueryAnswer::get_path_vector(unsigned int path_index) {
    if ((this->handles.size() == 0) || (path_index >= (this->handles.size() - 1))) {
        Utils::error("Invalid path index: " + std::to_string(path_index) +
                     " QueryAnswer: " + to_string());
    }
    return this->handles[path_index + 1];
}
