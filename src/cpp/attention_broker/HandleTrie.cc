#include "Utils.h"
#include "expression_hasher.h"
#include "HandleTrie.h"
#include <iostream>
#include <stack>

using namespace attention_broker_server;
using namespace commons;

HandleTrie::TrieValue::TrieValue() {
}

HandleTrie::TrieValue::~TrieValue() {
}

HandleTrie::TrieNode::TrieNode() {
    children = new TrieNode*[TRIE_ALPHABET_SIZE];
    for (unsigned int i = 0; i < TRIE_ALPHABET_SIZE; i++) {
        children[i] = NULL;
    }
    this->value = NULL;
    this->suffix_start = 0;
}

HandleTrie::TrieNode::~TrieNode() {
    for (unsigned int i = 0; i < TRIE_ALPHABET_SIZE; i++) {
        delete children[i];
    }
    delete [] children;
    delete value;
}

string HandleTrie::TrieValue::to_string() {
    return "";
}

bool HandleTrie::TLB_INITIALIZED = false;
unsigned char HandleTrie::TLB[256];

// --------------------------------------------------------------------------------
// Public methods


string HandleTrie::TrieNode::to_string() {
    string answer;
    if (suffix_start == 0) {
        answer = "''";
    } else {
        int n = suffix.size() - suffix_start;
        answer = suffix.substr(0, suffix_start) + "." + suffix.substr(suffix_start, n);
    }
    answer += " [";
    for (unsigned int i = 0; i < TRIE_ALPHABET_SIZE; i++) {
        if (children[i] != NULL) {
            answer += ("*");
        }
    }
    answer += "] ";
    if (value != NULL) {
        answer += value->to_string();
    }
    return answer;
}

HandleTrie::HandleTrie(unsigned int key_size) {
    if (key_size == 0 || key_size > 255) {
        Utils::error("Invalid key size: " + to_string(key_size));
    }
    this->key_size = key_size;
    if (! HandleTrie::TLB_INITIALIZED) {
        HandleTrie::TLB_INIT();
    }
    root = new TrieNode();
}

HandleTrie::~HandleTrie() {
    delete root;
}

HandleTrie::TrieValue *HandleTrie::insert(const string &key, TrieValue *value) {

    if (key.size() != key_size) {
        Utils::error("Invalid key size: " + to_string(key.size()) + " != " + to_string(key_size));
    }

    TrieNode *tree_cursor = root;
    TrieNode *parent = root;
    TrieNode *child;
    TrieNode *split;
    unsigned char key_cursor = 0;
    tree_cursor->trie_node_mutex.lock();
    while (true) {
        unsigned char c = TLB[(unsigned char) key[key_cursor]];
        if (tree_cursor->children[c] == NULL) {
            if (tree_cursor->suffix_start > 0) {
                unsigned char c_key_pred  = TLB[(unsigned char) key[key_cursor -1]];
                if (key[key_cursor] == tree_cursor->suffix[key_cursor]) {
                    child = new TrieNode();
                    child->trie_node_mutex.lock();
                    child->children[c] = tree_cursor;
                    tree_cursor->suffix_start++;
                    parent->children[c_key_pred] = child;
                    parent->trie_node_mutex.unlock();
                    parent = child;
                    key_cursor++;
                } else {
                    child = new TrieNode();
                    child->suffix = key;
                    child->suffix_start = key_cursor + 1;
                    child->value = value;
                    unsigned char c_tree_cursor = TLB[(unsigned char) tree_cursor->suffix[tree_cursor->suffix_start]];
                    tree_cursor->suffix_start++;
                    split = new TrieNode();
                    split->children[c] = child;
                    split->children[c_tree_cursor] = tree_cursor;
                    parent->children[c_key_pred] = split;
                    parent->trie_node_mutex.unlock();
                    if (tree_cursor != parent) {
                        tree_cursor->trie_node_mutex.unlock();
                    }
                    return child->value;
                }
            } else {
                child = new TrieNode();
                child->suffix = key;
                child->suffix_start = key_cursor + 1;
                child->value = value;
                tree_cursor->children[c] = child;
                parent->trie_node_mutex.unlock();
                if (tree_cursor != parent) {
                    tree_cursor->trie_node_mutex.unlock();
                }
                return child->value;
            }
        } else {
            if (tree_cursor != parent) {
                parent->trie_node_mutex.unlock();
                parent = tree_cursor;
            }
            tree_cursor = tree_cursor->children[c];
            tree_cursor->trie_node_mutex.lock();
            if (tree_cursor->suffix_start > 0) {
                bool match = true;
                unsigned int n = key.size();
                for (unsigned int i = key_cursor; i < n; i++) {
                    if (key[i] != tree_cursor->suffix[i]) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    tree_cursor->value->merge(value);
                    delete value;
                    if (tree_cursor != parent) {
                        parent->trie_node_mutex.unlock();
                    }
                    tree_cursor->trie_node_mutex.unlock();
                    return tree_cursor->value;
                }
            }
            key_cursor++;
        }
    }
}

HandleTrie::TrieValue *HandleTrie::lookup(const string &key) {

    if (key.size() != key_size) {
        Utils::error("Invalid key size: " + to_string(key.size()) + " != " + to_string(key_size));
    }

    TrieNode *tree_cursor = root;
    TrieValue *value;
    unsigned char key_cursor = 0;
    tree_cursor->trie_node_mutex.lock();
    while (tree_cursor != NULL) {
        if (tree_cursor->suffix_start > 0) {
            bool match = true;
            unsigned int n = key.size();
            for (unsigned int i = key_cursor; i < n; i++) {
                if (key[i] != tree_cursor->suffix[i]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                value = tree_cursor->value;
            } else {
                value = NULL;
            }
            tree_cursor->trie_node_mutex.unlock();
            return value;
        } else {
            unsigned char c = TLB[(unsigned char) key[key_cursor]];
            TrieNode *child = tree_cursor->children[c];
            tree_cursor->trie_node_mutex.unlock();
            tree_cursor = child;
            key_cursor++;
            if (tree_cursor != NULL) {
                tree_cursor->trie_node_mutex.lock();
            }
        }
    }
    return NULL;
}

void HandleTrie::traverse(bool keep_root_locked, bool (*visit_function)(TrieNode *node, void *data), void *data) {
    
    stack<TrieNode *> node_stack;
    TrieNode *cursor;
    node_stack.push(root);

    while (! node_stack.empty()) {
        cursor = node_stack.top();
        node_stack.pop();
        cursor->trie_node_mutex.lock();
        if (cursor->suffix_start > 0) {
            if (visit_function(cursor, data)) {
                if (keep_root_locked && (root != cursor)) {
                    root->trie_node_mutex.unlock();
                }
                cursor->trie_node_mutex.unlock();
                return;
            }
        } else {
            for (unsigned int i = TRIE_ALPHABET_SIZE - 1; ; i--) {
                if (cursor->children[i] != NULL) {
                    node_stack.push(cursor->children[i]);
                }
                if (i == 0) {
                    break;
                }
            }
        }
        if ((! keep_root_locked) || (cursor != root)) {
            cursor->trie_node_mutex.unlock();
        }
    }
    if (keep_root_locked) {
        root->trie_node_mutex.unlock();
    }
}
