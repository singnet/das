#include <iostream>
#include <stack>
#include "HebbianNetwork.h"
#include "Utils.h"
#include "expression_hasher.h"

using namespace attention_broker_server;

// --------------------------------------------------------------------------------
// Public methods

HebbianNetwork::HebbianNetwork() {
    nodes = new HandleTrie(HANDLE_HASH_SIZE - 1);
    largest_arity = 0;
    tokens_mutex.lock();
    tokens_to_distribute = 1.0;
    tokens_mutex.unlock();
}

HebbianNetwork::~HebbianNetwork() {
    delete nodes;
}

string HebbianNetwork::Node::to_string() {
    return "(" + std::to_string(count) + ", " + std::to_string(importance) +  ", " + std::to_string(arity) + ")";
}

string HebbianNetwork::Edge::to_string() {
    return "(" + std::to_string(count) + ")";
}

HebbianNetwork::Node *HebbianNetwork::add_node(string handle) {
    return (Node *) nodes->insert(handle, new HebbianNetwork::Node());
}

HebbianNetwork::Edge *HebbianNetwork::add_asymmetric_edge(string handle1, string handle2, Node *node1, Node *node2) {
    if (node1 == NULL) {
        node1 = (Node *) nodes->lookup(handle1);
    }
    Edge *edge = (Edge *) node1->neighbors->insert(handle2, new HebbianNetwork::Edge());
    if (edge->count == 1) {
        // First time this edge is added
        edge->node1 = node1;
        edge->node2 = node2;
        node1->arity += 1;
        largest_arity_mutex.lock();
        if (node1->arity > largest_arity) {
            largest_arity = node1->arity;
        }
        largest_arity_mutex.unlock();
    }
    return edge;
}

void HebbianNetwork::add_symmetric_edge(string handle1, string handle2, Node *node1, Node *node2) {
    add_asymmetric_edge(handle1, handle2, node1, node2);
    add_asymmetric_edge(handle2, handle1, node2, node1);
}

HebbianNetwork::Node *HebbianNetwork::lookup_node(string handle) {
    return (Node *) nodes->lookup(handle);
}

unsigned int HebbianNetwork::get_node_count(string handle) {
    Node *node = (Node *) nodes->lookup(handle);
    if (node == NULL) {
        return 0;
    } else {
        return node->count;
    }
}

ImportanceType HebbianNetwork::get_node_importance(string handle) {
    Node *node = (Node *) nodes->lookup(handle);
    if (node == NULL) {
        return 0;
    } else {
        return node->importance;
    }
}

unsigned int HebbianNetwork::get_asymmetric_edge_count(string handle1, string handle2) {
    Node *source = (Node *) nodes->lookup(handle1);
    if (source != NULL) {
        Edge *edge = (Edge *) source->neighbors->lookup(handle2);
        if (edge != NULL) {
            return edge->count;
        }
    }
    return 0;
}

ImportanceType HebbianNetwork::alienate_tokens() {
    ImportanceType answer;
    tokens_mutex.lock();
    answer = tokens_to_distribute;
    tokens_to_distribute = 0.0;
    tokens_mutex.unlock();
    return answer;
}

void HebbianNetwork::visit_nodes(
    bool keep_root_locked,
    bool (*visit_function)(HandleTrie::TrieNode *node, void *data),
    void *data) {

    nodes->traverse(keep_root_locked, visit_function, data);
}

static inline void release_locks(
    HandleTrie::TrieNode *root,
    HandleTrie::TrieNode *cursor,
    bool keep_root_locked,
    bool release_root_after_end) {

    if (keep_root_locked && release_root_after_end && (root != cursor)) {
        root->trie_node_mutex.unlock();
    }
    cursor->trie_node_mutex.unlock();
}
