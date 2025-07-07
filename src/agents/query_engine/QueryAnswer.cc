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
    this->handles_size = 0;
    this->strength = 0;
}

QueryAnswer::QueryAnswer(const char* handle, double importance) {
    this->importance = importance;
    this->handles[0] = handle;
    this->handles_size = 1;
    this->strength = 0;
}

QueryAnswer::~QueryAnswer() {}

void QueryAnswer::add_handle(const char* handle) { this->handles[this->handles_size++] = handle; }

QueryAnswer* QueryAnswer::copy(QueryAnswer* other) {  // Static method
    QueryAnswer* copy = new QueryAnswer(other->importance);
    copy->strength = other->strength;
    copy->assignment.copy_from(other->assignment);
    copy->handles_size = other->handles_size;
    memcpy((void*) copy->handles, (const void*) other->handles, other->handles_size * sizeof(char*));
    return copy;
}

bool QueryAnswer::merge(QueryAnswer* other, bool merge_handles) {
    if (this->assignment.is_compatible(other->assignment)) {
        this->assignment.add_assignments(other->assignment);
        bool already_exist;
        if (merge_handles) {
            this->importance = fmax(this->importance, other->importance);
            this->strength = this->strength * other->strength;
            for (unsigned int j = 0; j < other->handles_size; j++) {
                already_exist = false;
                for (unsigned int i = 0; i < this->handles_size; i++) {
                    if (strncmp(this->handles[i], other->handles[j], HANDLE_HASH_SIZE) == 0) {
                        already_exist = true;
                        break;
                    }
                }
                if (!already_exist) {
                    this->handles[this->handles_size++] = other->handles[j];
                }
            }
        }
        return true;
    } else {
        return false;
    }
}

string QueryAnswer::to_string() {
    string answer = "QueryAnswer<" + std::to_string(this->handles_size) + ",";
    answer += std::to_string(this->assignment.variable_count()) + "> [";
    for (unsigned int i = 0; i < this->handles_size; i++) {
        answer += string(this->handles[i]);
        if (i != (this->handles_size - 1)) {
            answer += ", ";
        }
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
    unsigned int char_count = 13    // strength with 10 decimals + space
                              + 13  // importance with 10 decimals + space
                              + 4   // (up to 3 digits) to represent this->handles_size + space
                              + this->handles_size * (HANDLE_HASH_SIZE + 1)  // handles + spaces
                              + 4  // (up to 3 digits) to represent this->assignment.size + space
                              + this->assignment.size * (MAX_VARIABLE_NAME_SIZE + HANDLE_HASH_SIZE +
                                                         2);  // label<space>handle<space>

    this->token_representation.clear();
    this->token_representation.reserve(char_count);
    string space = " ";
    this->token_representation += strength_buffer;
    this->token_representation += space;
    this->token_representation += importance_buffer;
    this->token_representation += space;
    this->token_representation += std::to_string(this->handles_size);
    this->token_representation += space;
    for (unsigned int i = 0; i < this->handles_size; i++) {
        this->token_representation += handles[i];
        this->token_representation += space;
    }
    this->token_representation += std::to_string(this->assignment.size);
    this->token_representation += space;
    for (unsigned int i = 0; i < this->assignment.size; i++) {
        this->token_representation += this->assignment.labels[i];
        this->token_representation += space;
        this->token_representation += this->assignment.values[i];
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
    this->handles_size = (unsigned int) std::stoi(number);
    if (this->handles_size > MAX_NUMBER_OF_OPERATION_CLAUSES) {
        Utils::error("Invalid handles_size: " + std::to_string(this->handles_size) +
                     " untokenizing QueryAnswer");
    }

    for (unsigned int i = 0; i < this->handles_size; i++) {
        read_token(token_string, cursor, handle, HANDLE_HASH_SIZE);
        this->handles[i] = strdup(handle);
    }

    read_token(token_string, cursor, number, 4);
    this->assignment.size = (unsigned int) std::stoi(number);

    if (this->assignment.size > MAX_NUMBER_OF_VARIABLES_IN_QUERY) {
        Utils::error("Invalid number of assignments: " + std::to_string(this->assignment.size) +
                     " untokenizing QueryAnswer");
    }

    for (unsigned int i = 0; i < this->assignment.size; i++) {
        read_token(token_string, cursor, label, MAX_VARIABLE_NAME_SIZE);
        read_token(token_string, cursor, handle, HANDLE_HASH_SIZE);
        this->assignment.labels[i] = strdup(label);
        this->assignment.values[i] = strdup(handle);
    }

    if (token_string[cursor] != '\0') {
        Utils::error("Invalid token string - invalid text after QueryAnswer definition");
    }
}
