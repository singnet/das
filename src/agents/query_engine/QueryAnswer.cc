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
    this->handles.insert(handle);
    this->strength = 0;
}

QueryAnswer::~QueryAnswer() {}

void QueryAnswer::add_handle(const string& handle) { this->handles.insert(handle); }

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
        bool already_exist;
        if (merge_handles) {
            this->importance = fmax(this->importance, other->importance);
            this->strength = this->strength * other->strength;
            this->handles.insert(other.handles.begin(), other.handles.end());
        }
        return true;
    } else {
        return false;
    }
}

string QueryAnswer::to_string() {
    string answer = "QueryAnswer<" + std::to_string(this->handles.size()) + ",";
    answer += std::to_string(this->assignment.variable_count()) + "> [";
    for (string handle: this->handles) {
        answer += handle;
        answer += ", ";
    }
    answer.pop_back();
    answer.pop_back();
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
    unsigned int char_count = 13    // strength with 10 decimals + space
                              + 13  // importance with 10 decimals + space
                              + 4   // (up to 3 digits) to represent this->handles.size() + space
                              + this->handles_size * (HANDLE_HASH_SIZE + 1)  // handles + spaces
                              + 4  // (up to 3 digits) to represent this->assignment.size + space
                              + this->assignment.table.size() * (MAX_VARIABLE_NAME_SIZE + HANDLE_HASH_SIZE + 2);  // label<space>handle<space>

    this->token_representation.clear();
    this->token_representation.reserve(char_count);
    string space = " ";
    this->token_representation += strength_buffer;
    this->token_representation += space;
    this->token_representation += importance_buffer;
    this->token_representation += space;
    this->token_representation += std::to_string(this->handles.size());
    this->token_representation += space;
    for (string handle: this->handles) {
        this->token_representation += handle;
        this->token_representation += space;
    }
    this->token_representation += std::to_string(this->assignment.table.size());
    this->token_representation += space;
    for (auto pair: this->assignment.table) {
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
        this->handles.insert(string(handle));
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

    if (token_string[cursor] != '\0') {
        Utils::error("Invalid token string - invalid text after QueryAnswer definition");
    }
}
