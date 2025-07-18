#include "Assignment.h"

#include <cstring>

#include "Hasher.h"
#include "Utils.h"

using namespace commons;

string Assignment::EMPTY_VALUE = "";

Assignment::Assignment() {}

Assignment::~Assignment() {}

bool Assignment::assign(const string& label, const string& value) {
    auto iterator = this->table.find(label);
    if (iterator == this->table.end()) {
        // label is not present, so makes the assignment and return true
        if (label.size() > MAX_VARIABLE_NAME_SIZE) {
            Utils::error("Invalid assignment. Label size (" + std::to_string(label.size()) + ") is too large (> " + std::to_string(MAX_VARIABLE_NAME_SIZE) + ").");
        }
        this->table[label] = value;
        if (this->table.size() > MAX_NUMBER_OF_VARIABLES_IN_QUERY) {
            Utils::error("Assignment size exceeds the maximal number of allowed variables in a query: " +
                         std::to_string(MAX_NUMBER_OF_VARIABLES_IN_QUERY));
        }
        return true;
    } else {
        // if label is already present, return true iff its value is the same
        return (iterator->second == value);
    }
}

bool Assignment::is_compatible(const Assignment& other) {
    for (auto pair: this->table) {
        auto iterator = other.table.find(pair.first);
        if (iterator != other.table.end()) {
            if (iterator->second != pair.second) {
                return false;
            }
        }
    }
    return true;
}

void Assignment::copy_from(const Assignment& other) {
    this->table = other.table;
}

void Assignment::add_assignments(const Assignment& other) {
    for (auto pair: other.table) {
        if (this->table.find(pair.first) == this->table.end()) {
            this->table[pair.first] = pair.second;
        }
    }
}

const string& Assignment::get(const string& label) {
    auto iterator = this->table.find(label);
    if (iterator != this->table.end()) {
        return iterator->second;
    } else {
        return EMPTY_VALUE;
    }
}

unsigned int Assignment::variable_count() { return this->table.size(); }

string Assignment::to_string() {
    string answer = "{";
    for (auto pair: this->table) {
        answer += "(" + pair.first + ": " + pair.second + ")";
        answer += ", ";
    }
    answer.pop_back();
    answer.pop_back();
    answer += "}";
    return answer;
}

void Assignment::clear() { this->table.clear(); }

bool Assignment::operator==(const Assignment& other) const {
    return (this->table == other.table);
}
