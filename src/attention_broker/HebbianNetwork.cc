#include "HebbianNetwork.h"

#include <iostream>
#include <stack>

#include "Utils.h"
#include "expression_hasher.h"

using namespace attention_broker;

// --------------------------------------------------------------------------------
// Public methods

HebbianNetwork::HebbianNetwork() {
    nodes = new HandleTrie(HANDLE_HASH_SIZE - 1);
    largest_arity = 0;
    tokens_mutex.lock();
    tokens_to_distribute = 1.0;
    tokens_mutex.unlock();
}

HebbianNetwork::~HebbianNetwork() { delete nodes; }

string HebbianNetwork::Node::to_string() {
    return "(" + std::to_string(count) + ", " + std::to_string(importance) + ", " +
           std::to_string(arity) + ")";
}

string HebbianNetwork::Edge::to_string() { return "(" + std::to_string(count) + ")"; }

HebbianNetwork::Node* HebbianNetwork::add_node(string handle) {
    return (Node*) nodes->insert(handle, new HebbianNetwork::Node());
}

HebbianNetwork::Edge* HebbianNetwork::add_asymmetric_edge(string handle1,
                                                          string handle2,
                                                          Node* node1,
                                                          Node* node2) {
    if (node1 == NULL) {
        node1 = (Node*) nodes->lookup(handle1);
    }
    Edge* edge = (Edge*) node1->neighbors->insert(handle2, new HebbianNetwork::Edge());
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

void HebbianNetwork::add_symmetric_edge(string handle1, string handle2, Node* node1, Node* node2) {
    add_asymmetric_edge(handle1, handle2, node1, node2);
    add_asymmetric_edge(handle2, handle1, node2, node1);
}

HebbianNetwork::Node* HebbianNetwork::lookup_node(string handle) {
    return (Node*) nodes->lookup(handle);
}

unsigned int HebbianNetwork::get_node_count(string handle) {
    Node* node = (Node*) nodes->lookup(handle);
    if (node == NULL) {
        return 0;
    } else {
        return node->count;
    }
}

ImportanceType HebbianNetwork::get_node_importance(string handle) {
    Node* node = (Node*) nodes->lookup(handle);
    if (node == NULL) {
        return 0;
    } else {
        return node->get_importance();
    }
}

unsigned int HebbianNetwork::get_asymmetric_edge_count(string handle1, string handle2) {
    Node* source = (Node*) nodes->lookup(handle1);
    if (source != NULL) {
        Edge* edge = (Edge*) source->neighbors->lookup(handle2);
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

void HebbianNetwork::visit_nodes(bool keep_root_locked,
                                 bool (*visit_function)(HandleTrie::TrieNode* node, void* data),
                                 void* data) {
    nodes->traverse(keep_root_locked, visit_function, data);
}

static inline void release_locks(HandleTrie::TrieNode* root,
                                 HandleTrie::TrieNode* cursor,
                                 bool keep_root_locked,
                                 bool release_root_after_end) {
    if (keep_root_locked && release_root_after_end && (root != cursor)) {
        root->trie_node_mutex.unlock();
    }
    cursor->trie_node_mutex.unlock();
}

// --------------------------------------------------------------------------------
// Serialization/deserialization methods

struct VisitData {
    HebbianNetwork::Node* node;
    HebbianNetwork* network;
    string handle;
    ostream* stream;
};

static bool visit_find_handle(HandleTrie::TrieNode* trie_node, void* data) {
    VisitData* visit_data = (VisitData *) data;
    HebbianNetwork::Node* node = (HebbianNetwork::Node*) trie_node->value;
    if (node == visit_data->node) {
        visit_data->handle = trie_node->suffix;
        return true;
    } else {
        return false;
    }
}

static bool visit_serialize_node(HandleTrie::TrieNode* trie_node, void* data) {
    HebbianNetwork::Node* node = (HebbianNetwork::Node*) trie_node->value;
    VisitData* visit_data = (VisitData *) data;
    visit_data->stream->write(trie_node->suffix.c_str(), HANDLE_HASH_SIZE - 1);
    visit_data->stream->write(reinterpret_cast<const char*>(&node->importance), sizeof(node->importance));
    visit_data->stream->write(reinterpret_cast<const char*>(&node->stimuli_to_spread), sizeof(node->stimuli_to_spread));
    unsigned int determiners_size = node->determiners.size();
    unsigned int neighbors_size = node->neighbors->size;
    visit_data->stream->write(reinterpret_cast<const char*>(&determiners_size), sizeof(determiners_size));
    visit_data->stream->write(reinterpret_cast<const char*>(&neighbors_size), sizeof(neighbors_size));
    return false;
}

static bool visit_serialize_determiners(HandleTrie::TrieNode* trie_node, void* data) {
    HebbianNetwork::Node* node = (HebbianNetwork::Node*) trie_node->value;
    VisitData* visit_data = (VisitData *) data;
    for (auto determiner : node->determiners) {
        visit_data->stream->write(trie_node->suffix.c_str(), HANDLE_HASH_SIZE - 1);
        visit_data.handle = "";
        visit_data.node = determiner;
        visit_data->network->visit_nodes(false, &visit_find_handle, (void *) &visit_data);
        if (visit_data.handle == "") {
            Utils::error("Invalid determiner");
        }
        visit_data->stream->write(visit_data.handle.c_str(), HANDLE_HASH_SIZE - 1);
    }
    return false;
}

static bool visit_serialize_edge(HandleTrie::TrieNode* trie_node, void* data) {
    HebbianNetwork::Edge* edge = (HebbianNetwork::Edge*) trie_node->value;
    VisitData* visit_data = (VisitData *) data;
    visit_data.handle = "";
    visit_data.node = edge->node1;
    visit_data->network->visit_nodes(false, &visit_find_handle, (void *) &visit_data);
    if (visit_data.handle == "") {
        Utils::error("Invalid determiner");
    }
    visit_data->stream->write(visit_data.handle.c_str(), HANDLE_HASH_SIZE - 1);
    visit_data->stream->write(trie_node->suffix.c_str(), HANDLE_HASH_SIZE - 1);
    return false;
}

static bool visit_serialize_neighbors(HandleTrie::TrieNode* trie_node, void* data) {
    HebbianNetwork::Node* node = (HebbianNetwork::Node*) trie_node->value;
    node->neighbors->traverse(false, &HebbianNetwork::visit_serialize_edge, data);
    return false;
}

void HebbianNetwork::deserialize_node(istream& is, unsigned int& determiner_count, unsigned int& neighbors_count) {
    string key;
    key.resize(HANDLE_HASH_SIZE - 1);
    is.read(&key[0], HANDLE_HASH_SIZE - 1);
    HebbianNetwork::Node *node = add_node(key);
    is.read(reinterpret_cast<char*>(&node->importance), sizeof(node->importance));
    is.read(reinterpret_cast<char*>(&node->stimuli_to_spread), sizeof(node->stimuli_to_spread));
    unsigned int n;
    is.read(reinterpret_cast<char*>(&n), sizeof(n));
    determiner_count += n;
    is.read(reinterpret_cast<char*>(&n), sizeof(n));
    neighbors_count += n;
}

void HebbianNetwork::serialize(ostream& os) const {
    os.write(reinterpret_cast<const char*>(&this->tokens_to_distribute), sizeof(this->tokens_to_distribute));
    os.write(reinterpret_cast<const char*>(&this->nodes->size), sizeof(this->nodes->size));
    VisitData visit_data;
    visit_data.node = NULL;
    visit_data.network = this;
    visit_data.handle = "";
    visit_data.stream = &os;
    visit_nodes(false, &HebbianNetwork::visit_serialize_node, &visit_data);
    visit_nodes(false, &HebbianNetwork::visit_serialize_determiners, &visit_data);
    visit_nodes(false, &HebbianNetwork::visit_serialize_neighbors, &visit_data);
}

void HebbianNetwork::deserialize(istream& is) {
    is.read(reinterpret_cast<char*>(&this->tokens_to_distribute), sizeof(this->tokens_to_distribute));
    unsigned int node_count;
    is.read(reinterpret_cast<char*>(&node_count), sizeof(node_count));
    unsigned int determiner_count = 0;
    unsigned int neighbors_count = 0;
    for (unsigned int i = 0; i < node_count; i++) {
        deserialize_node(is, determiner_count, neighbors_count);
    }
    string key1, key2;
    key1.resize(HANDLE_HASH_SIZE - 1);
    key2.resize(HANDLE_HASH_SIZE - 1);
    string last_key1 = "";
    HebbianNetwork::Node *node = NULL;
    for (unsigned int i = 0; i < determiner_count; i++) {
        is.read(&key1[0], HANDLE_HASH_SIZE - 1);
        is.read(&key2[0], HANDLE_HASH_SIZE - 1);
        if (key1 != last_key1) {
            node = nodes->lookup(key1);
            last_key1 = key1;
        }
        node->determiners.insert(nodes->lookup(key2));
    }
    last_key = "";
    HebbianNetwork::Node *node1;
    HebbianNetwork::Node *node2;
    for (unsigned int i = 0; i < neighbors_count; i++) {
        is.read(&key1[0], HANDLE_HASH_SIZE - 1);
        is.read(&key2[0], HANDLE_HASH_SIZE - 1);
        if (key1 != last_key1) {
            node1 = nodes->lookup(key1);
            last_key1 = key1;
        }
        node2 = nodes->lookup(key2);
        add_asymmetric_edge(key1, key2, node1, node2);
    }
}
