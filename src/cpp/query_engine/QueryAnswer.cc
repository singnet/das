#include "QueryAnswer.h"
#include "Utils.h"
#include <cmath>
#include <cstring>
#include <iostream>

using namespace query_engine;
using namespace commons;

// -------------------------------------------------------------------------------------------------
// Assignment

Assignment::Assignment() {
    this->size = 0;
}

Assignment::~Assignment() {
}

bool Assignment::assign(const char *label, const char *value) {
    for (unsigned int i = 0; i < this->size; i++) {
        // if label is already present, return true iff its value is the same
        if (strncmp(label, this->labels[i], MAX_VARIABLE_NAME_SIZE) == 0) {
            return (strncmp(value, this->values[i], HANDLE_HASH_SIZE) == 0);
        }
    }
    // label is not present, so makes the assignment and return true
    labels[this->size] = label;
    values[this->size] = value;
    this->size++;
    if (this->size == MAX_NUMBER_OF_VARIABLES_IN_QUERY) {
        Utils::error(
            "Assignment size exceeds the maximal number of allowed variables in a query: " +
            std::to_string(MAX_NUMBER_OF_VARIABLES_IN_QUERY));
    }
    return true;
}

bool Assignment::is_compatible(const Assignment &other) {
    for (unsigned int i = 0; i < this->size; i++) {
        for (unsigned int j = 0; j < other.size; j++) {
            if ((strncmp(this->labels[i], other.labels[j], MAX_VARIABLE_NAME_SIZE) == 0) &&
                (strncmp(this->values[i], other.values[j], HANDLE_HASH_SIZE) != 0)) {
                   return false;
            }
        }
    }
    return true;
}

void Assignment::copy_from(const Assignment &other) {
    this->size = other.size;
    unsigned int num_bytes = this->size * sizeof(char *);
    memcpy((void *) this->labels, (const void *) other.labels, num_bytes);
    memcpy((void *) this->values, (const void *) other.values, num_bytes);
}

void Assignment::add_assignments(const Assignment &other) {
    bool already_contains;
    for (unsigned int j = 0; j < other.size; j++) {
        already_contains = false;
        for (unsigned int i = 0; i < this->size; i++) {
            if (strncmp(this->labels[i], other.labels[j], MAX_VARIABLE_NAME_SIZE) == 0) {
                already_contains = true;
                break;
            }
        }
        if (! already_contains) {
            this->labels[this->size] = other.labels[j];
            this->values[this->size] = other.values[j];
            this->size++;
        }
    }
}

const char *Assignment::get(const char *label) {
    for (unsigned int i = 0; i < this->size; i++) {
        if (strncmp(label, this->labels[i], MAX_VARIABLE_NAME_SIZE) == 0) {
            return this->values[i];
        }
    }
    return NULL;
}

unsigned int Assignment::variable_count() {
    return this->size;
}

string Assignment::to_string() {
    string answer = "{";
    for (unsigned int i = 0; i < this->size; i++) {
        answer += "(" + string(this->labels[i]) + ": " + string(this->values[i]) + ")";
        if (i != (this->size - 1)) {
            answer += ", ";
        }
    }
    answer += "}";
    return answer;
}

// -------------------------------------------------------------------------------------------------
// QueryAnswer


QueryAnswer::QueryAnswer() : QueryAnswer(0.0) {
}

QueryAnswer::QueryAnswer(double importance) {
    this->importance = importance;
    this->handles_size = 0;
}

QueryAnswer::QueryAnswer(const char *handle, double importance) {
    this->importance = importance;
    this->handles[0] = handle;
    this->handles_size = 1;
}

QueryAnswer::~QueryAnswer() {
}

void QueryAnswer::add_handle(const char *handle) {
    this->handles[this->handles_size++] = handle;
}

QueryAnswer *QueryAnswer::copy(QueryAnswer *base) { // Static method
    QueryAnswer *copy = new QueryAnswer(base->importance);
    copy->assignment.copy_from(base->assignment);
    copy->handles_size = base->handles_size;
    memcpy(
        (void *) copy->handles, 
        (const void *) base->handles, 
        base->handles_size * sizeof(char *));
    return copy;
}

bool QueryAnswer::merge(QueryAnswer *other, bool merge_handles) {
    if (this->assignment.is_compatible(other->assignment)) {
        this->assignment.add_assignments(other->assignment);
        bool already_exist;
        if (merge_handles) {
            this->importance = fmax(this->importance, other->importance);
            for (unsigned int j = 0; j < other->handles_size; j++) {
                already_exist = false;
                for (unsigned int i = 0; i < this->handles_size; i++) {
                    if (strncmp(this->handles[i], other->handles[j], HANDLE_HASH_SIZE) == 0) {
                        already_exist = true;
                        break;
                    }
                }
                if (! already_exist) {
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
    answer += "] " + this->assignment.to_string() + " " + std::to_string(this->importance);
    //answer += "] " + this->assignment.to_string();
    return answer;
}

const string &QueryAnswer::tokenize() {
    // char_count is computed to be slightly larger than actually required by assuming
    // e.g. 3 digits to represent sizes
    char importance_buffer[13];
    sprintf(importance_buffer, "%.10f", this->importance);
    unsigned int char_count = 
        13 // importance with 10 decimals + space
        + 4 // (up to 3 digits) to represent this->handles_size + space
        + this->handles_size * (HANDLE_HASH_SIZE + 1) // handles + spaces
        + 4 // (up to 3 digits) to represent this->assignment.size + space
        + this->assignment.size * (MAX_VARIABLE_NAME_SIZE + HANDLE_HASH_SIZE + 2); // label<space>handle<space>

    this->token_representation.clear();
    this->token_representation.reserve(char_count);
    string space = " ";
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

static inline void read_token(
    const char *token_string, 
    unsigned int &cursor, 
    char *token, 
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

void QueryAnswer::untokenize(const string &tokens) {

    const char *token_string = tokens.c_str();
    char number[4];
    char importance[13];
    char handle[HANDLE_HASH_SIZE];
    char label[MAX_VARIABLE_NAME_SIZE];

    unsigned int cursor = 0;

    read_token(token_string, cursor, importance, 13);
    this->importance = std::stod(importance);

    read_token(token_string, cursor, number, 4);
    this->handles_size = (unsigned int) std::stoi(number);
    if (this->handles_size > MAX_NUMBER_OF_OPERATION_CLAUSES) {
        Utils::error("Invalid handles_size: " + std::to_string(this->handles_size) + " untokenizing QueryAnswer");
    }

    for (unsigned int i = 0; i < this->handles_size; i++) {
        read_token(token_string, cursor, handle, HANDLE_HASH_SIZE);
        this->handles[i] = strdup(handle);
    }

    read_token(token_string, cursor, number, 4);
    this->assignment.size = (unsigned int) std::stoi(number);

    if (this->assignment.size > MAX_NUMBER_OF_VARIABLES_IN_QUERY) {
        Utils::error("Invalid number of assignments: " + std::to_string(this->assignment.size) + " untokenizing QueryAnswer");
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
