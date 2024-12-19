#include "expression_hasher.h"
#include "Utils.h"
#include "test_utils.h"
#include <algorithm>
#include <iostream>

static char REVERSE_TLB[16] = {
    '0',
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    'a',
    'b',
    'c',
    'd',
    'e',
    'f',
};

double random_importance() {
    return ((double) rand()) / RAND_MAX;
}

string random_handle() {
    char buffer[HANDLE_HASH_SIZE];
    unsigned int key_size = HANDLE_HASH_SIZE - 1;
    for (unsigned int i = 0; i < key_size; i++) {
        buffer[i] = REVERSE_TLB[(rand() % 16)];
    }
    buffer[key_size] = 0;
    string s = buffer;
    return s;
}

string sequential_label(unsigned int &count, string prefix) {
    return prefix + std::to_string(count++);
}

string prefixed_random_handle(string prefix) {
    char buffer[HANDLE_HASH_SIZE];
    unsigned int key_size = HANDLE_HASH_SIZE - 1;
    for (unsigned int i = 0; i < key_size; i++) {
        if (i < prefix.size()) {
            buffer[i] = prefix[i];
        } else {
            buffer[i] = REVERSE_TLB[(rand() % 16)];
        }
    }
    buffer[key_size] = 0;
    string s = buffer;
    return s;
}

static bool str_comp(string &a, string &b) {
    return a.compare(b) < 0;
}

string *build_handle_space(unsigned int size, bool sort) {
    string *answer = new string[size];
    for (unsigned int i = 0; i < size; i++) {
        answer[i] = random_handle();
    }
    if (sort) {
        std::sort(answer, answer + size, str_comp);
    }
    return answer;
}

bool double_equals(double v1, double v2) {
    return fabs(v2 - v1) < 0.001;
}
