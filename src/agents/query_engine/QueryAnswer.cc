#include "QueryAnswer.h"

#include <cmath>
#include <cstring>
#include <iostream>

#include "Utils.h"

using namespace query_engine;
using namespace commons;

QueryAnswer::QueryAnswer() : QueryAnswer(0.0) {}

QueryAnswer::QueryAnswer(double importance) {
    this->importance = importance;
    this->strength = 0;
}

QueryAnswer::QueryAnswer(const string& handle, double importance) {
    this->importance = importance;
    this->handles.push_back(handle);
    this->strength = 0;
}

QueryAnswer::~QueryAnswer() {}

void QueryAnswer::add_handle(const string& handle) { this->handles.push_back(handle); }

QueryAnswer* QueryAnswer::copy(QueryAnswer* other) {  // Static method
    QueryAnswer* copy = new QueryAnswer(other->importance);
    copy->strength = other->strength;
    copy->assignment = other->assignment;
    copy->handles = other->handles;
    return copy;
}

bool QueryAnswer::merge(QueryAnswer* other, bool merge_handles) {
    if (this->assignment.is_compatible(other->assignment)) {
        this->assignment.add_assignments(other->assignment);
        if (merge_handles) {
            this->importance = fmax(this->importance, other->importance);
            this->strength = this->strength * other->strength;
            for (string handle1 : other->handles) {
                bool flag = true;
                for (string handle2 : this->handles) {
                    if (handle1 == handle2) {
                        flag = false;
                    }
                }
                if (flag) {
                    this->handles.push_back(handle1);
                }
            }
        }
        return true;
    } else {
        return false;
    }
}

string QueryAnswer::to_string() {
    string answer = "QueryAnswer<" + std::to_string(this->handles.size()) + ",";
    answer += std::to_string(this->assignment.variable_count()) + "> [";
    bool empty_flag = true;
    for (string handle : this->handles) {
        answer += handle;
        answer += ", ";
        empty_flag = false;
    }
    if (!empty_flag) {
        answer.pop_back();
        answer.pop_back();
    }
    answer += "] " + this->assignment.to_string();
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
        + 4   // (up to 3 digits) to represent this->handles.size() + space
        + this->handles.size() * (HANDLE_HASH_SIZE + 1)  // handles + spaces
        + 4  // (up to 3 digits) to represent this->assignment.size + space
        + this->assignment.table.size() *
              (MAX_VARIABLE_NAME_SIZE + HANDLE_HASH_SIZE + 2)  // label<space>handle<space>
        + 4;  // (up to 3 digits) to represent this->metta_expression.size + space

    this->token_representation.clear();
    this->token_representation.reserve(char_count);
    string space = " ";
    this->token_representation += strength_buffer;
    this->token_representation += space;
    this->token_representation += importance_buffer;
    this->token_representation += space;
    this->token_representation += std::to_string(this->handles.size());
    this->token_representation += space;
    for (string handle : this->handles) {
        this->token_representation += handle;
        this->token_representation += space;
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
    if (handles_size > MAX_NUMBER_OF_OPERATION_CLAUSES) {
        Utils::error("Invalid handles_size: " + std::to_string(handles_size) +
                     " untokenizing QueryAnswer");
    }

    for (unsigned int i = 0; i < handles_size; i++) {
        read_token(token_string, cursor, handle, HANDLE_HASH_SIZE);
        this->handles.push_back(string(handle));
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

string QueryAnswer::get(const QueryAnswerElement& element_key) {
    string answer = "";
    switch (element_key.type) {
        case QueryAnswerElement::HANDLE: {
            if (element_key.index < this->handles.size()) {
                answer = this->handles[element_key.index];
            } else {
                Utils::error("Invalid handle index: " + std::to_string(element_key.index));
            }
            break;
        }
        case QueryAnswerElement::VARIABLE: {
            answer = this->assignment.get(element_key.name);
            if (answer == "") {
                Utils::error("Invalid variable name: " + element_key.name);
            }
            break;
        }
        default:
            Utils::error("Invalid QueryAnswerElement type: " + std::to_string(element_key.type));
    }
    return answer;
}
