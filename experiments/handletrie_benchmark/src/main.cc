#include <cstdlib>
#include <thread>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <string.h>
#include <map>
#include <algorithm>
#include <numeric>

#include "Utils.h"
#include "expression_hasher.h"
#include "HandleTrie.h"

using namespace attention_broker_server;
using namespace std;

#define HANDLE_SPACE_SIZE ((unsigned int)100)

using namespace attention_broker_server;
using namespace std;

template <typename T>
T variance(const std::vector<T> &vec)
{
    const size_t sz = vec.size();
    if (sz <= 1)
    {
        return 0.0;
    }

    // Calculate the mean
    const T mean = std::accumulate(vec.begin(), vec.end(), 0.0) / sz;

    // Now calculate the variance
    auto variance_func = [&mean, &sz](T accumulator, const T &val)
    {
        return accumulator + ((val - mean) * (val - mean) / (sz - 1));
    };

    return std::accumulate(vec.begin(), vec.end(), 0.0, variance_func);
}

class TestValue : public HandleTrie::TrieValue
{
public:
    unsigned int count;
    TestValue(int count = 1)
    {
        this->count = count;
    }
    void merge(TrieValue *other)
    {
    }

    std::string to_string()
    {
        return std::to_string(this->count);
    }
};

char R_TLB[16] = {
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

void benchmark_handletrie(unsigned int n_insertions = 1000000)
{
    char buffer[1000];
    TestValue *value;
    for (unsigned int key_count : {1, 2, 5})
    {
        unsigned int key_size = (HANDLE_HASH_SIZE - 1) * key_count;
        HandleTrie *trie = new HandleTrie(key_size);
        for (unsigned int i = 0; i < n_insertions; i++)
        {
            for (unsigned int j = 0; j < key_size; j++)
            {
                buffer[j] = R_TLB[(rand() % 16)];
            }
            buffer[key_size] = 0;
            string s = buffer;
            value = (TestValue *)trie->lookup(s);
            if (value == NULL)
            {
                value = new TestValue();
                trie->insert(s, value);
            }
            else
            {
                value->count += 1;
            }
        }
        delete trie;
    }
}

void benchmark_none(unsigned int n_insertions = 1000000)
{
    char buffer[1000];
    for (unsigned int key_count : {1, 2, 5})
    {
        unsigned int key_size = (HANDLE_HASH_SIZE - 1) * key_count;
        for (unsigned int i = 0; i < n_insertions; i++)
        {
            for (unsigned int j = 0; j < key_size; j++)
            {
                buffer[j] = R_TLB[(rand() % 16)];
            }
            buffer[key_size] = 0;
            string s = buffer;
        }
    }
}

void benchmark_map(unsigned int n_insertions = 1000000)
{
    char buffer[1000];
    map<string, int> baseline;
    for (unsigned int key_count : {1, 2, 5})
    {
        unsigned int key_size = (HANDLE_HASH_SIZE - 1) * key_count;
        for (unsigned int i = 0; i < n_insertions; i++)
        {
            for (unsigned int j = 0; j < key_size; j++)
            {
                buffer[j] = R_TLB[(rand() % 16)];
            }
            buffer[key_size] = 0;
            string s = buffer;
            if (baseline.find(s) == baseline.end())
            {
                baseline[s] = 0;
            }
            baseline[s] = baseline[s] + 1;
        }
    }
}


void benchmark_unordered_map(unsigned int n_insertions = 1000000)
{
    char buffer[1000];
    unordered_map<string, int> baseline;
    for (unsigned int key_count : {1, 2, 5})
    {
        unsigned int key_size = (HANDLE_HASH_SIZE - 1) * key_count;
        for (unsigned int i = 0; i < n_insertions; i++)
        {
            for (unsigned int j = 0; j < key_size; j++)
            {
                buffer[j] = R_TLB[(rand() % 16)];
            }
            buffer[key_size] = 0;
            string s = buffer;
            if (baseline.find(s) == baseline.end())
            {
                baseline[s] = 0;
            }
            baseline[s] = baseline[s] + 1;
        }
    }
}

int main(int argc, char *argv[])
{

    string option = argv[1];
    if (option == "handletrie")
    {
        benchmark_handletrie(atoi(argv[2]));
    }
    if (option == "none")
    {
        benchmark_none(atoi(argv[2]));
    }
    if (option == "map")
    {
        benchmark_map(atoi(argv[2]));
    }
    if (option == "unordered_map")
    {
        benchmark_map(atoi(argv[2]));
    }

    return 0;
}