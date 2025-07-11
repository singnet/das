#include "Assignment.h"

#include <cstring>

#include "Hasher.h"
#include "Utils.h"

using namespace commons;

Assignment::Assignment() { this->size = 0; }

Assignment::~Assignment() {}

bool Assignment::assign(const char* label, const char* value) {
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
        Utils::error("Assignment size exceeds the maximal number of allowed variables in a query: " +
                     std::to_string(MAX_NUMBER_OF_VARIABLES_IN_QUERY));
    }
    return true;
}

bool Assignment::is_compatible(const Assignment& other) {
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

void Assignment::copy_from(const Assignment& other) {
    this->size = other.size;
    unsigned int num_bytes = this->size * sizeof(char*);
    memcpy((void*) this->labels, (const void*) other.labels, num_bytes);
    memcpy((void*) this->values, (const void*) other.values, num_bytes);
}

void Assignment::add_assignments(const Assignment& other) {
    bool already_contains;
    for (unsigned int j = 0; j < other.size; j++) {
        already_contains = false;
        for (unsigned int i = 0; i < this->size; i++) {
            if (strncmp(this->labels[i], other.labels[j], MAX_VARIABLE_NAME_SIZE) == 0) {
                already_contains = true;
                break;
            }
        }
        if (!already_contains) {
            this->labels[this->size] = other.labels[j];
            this->values[this->size] = other.values[j];
            this->size++;
        }
    }
}

const char* Assignment::get(const char* label) {
    for (unsigned int i = 0; i < this->size; i++) {
        if (strncmp(label, this->labels[i], MAX_VARIABLE_NAME_SIZE) == 0) {
            return this->values[i];
        }
    }
    return NULL;
}

unsigned int Assignment::variable_count() { return this->size; }

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

void Assignment::clear() { this->size = 0; }

bool Assignment::operator==(const Assignment& other) const {
    if (this->size != other.size) {
        return false;
    }
    unsigned int n = this->size;
    for (unsigned int i = 0; i < n; i++) {
        unsigned int j = 0;
        while ((j != n) && strncmp(this->labels[i], other.labels[j], MAX_VARIABLE_NAME_SIZE)) {
            j++;
        }
        if (j == n) {
            // There's a variable in "this" which doesn't exist in "other"
            return false;
        } else if (strncmp(this->values[i], other.values[j], HANDLE_HASH_SIZE)) {
            // There same variable have different values in "this" and "other"
            return false;
        }
    }
    return true;
}
