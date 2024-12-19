#ifndef _ATTENTION_BROKER_SERVER_HEBBIANNETWORK_H
#define _ATTENTION_BROKER_SERVER_HEBBIANNETWORK_H

#include <string>
#include <mutex>
#include <forward_list>
#include "HandleTrie.h"
#include "expression_hasher.h"

using namespace std;

namespace attention_broker_server {

typedef double ImportanceType;

/**
 * Data abstraction of an asymmetric Hebbian Network with only direct hebbian links.
 *
 * A Hebbian Network, in the context of AttentionBrokerServer, is a directed graph with
 * weights in the edges A->B representing the probability of B being present in a DAS
 * query answer given that A is present. In other words, provided that the atom A is one of
 * the atoms returned in a given query in DAS, the weight of the edge A->B in 
 * a HebbianNetwork is an estimate of the probability of B being present in the same answer.
 *
 * HebbianNetwork is asymmetric, meaning that the weight of A->B can be different from the
 * weight of B->A. 
 *
 * All edges in HebbianNetwork are "direct" hebbian links meaning that they estimate the
 * probability of B BEING present in a answer given that A is. There are no "reverse"
 * hebbian links which would mean the probability of B being NOT present in an answer 
 * given that A is.
 *
 * HebbianNetwork keeps nodes in a HandleTrie which is basically a data structure to map
 * from handle to a value object (mostly like a hashmap but slightly more efficient because
 * it makes the assumption that the key is a handle). So in the stored value object it stores
 * another HandleTrie to keep track of the neighbors. So we have two types of value objects,
 * one to represent Nodes and another one to represent Edges.
 */
class HebbianNetwork {

public:

    HebbianNetwork(); /// Basic constructor.
    ~HebbianNetwork(); /// Destructor.

    unsigned int largest_arity; /// Largest arity among nodes in this network.
    mutex largest_arity_mutex;

    // Node and Link don't inherit from a common "Atom" class to avoid having virtual methods,
    // which couldn't be properly inlined.

    /**
     * Node object used as the value in HandleTrie.
     */
    class Node: public HandleTrie::TrieValue {
        public:
        unsigned int arity; /// Number of neighbors of this Node.
        unsigned int count; /// Count for this Node.
        ImportanceType importance; /// Importance of this Node.
        ImportanceType stimuli_to_spread; /// Amount of importance this node will spread in the next
                                          /// stimuli spreading cycle.
        HandleTrie *neighbors; // Neighbors of this Node.
        Node() {
            arity = 0;
            count = 1;
            importance = 0.0;
            stimuli_to_spread = 0.0;
            neighbors = new HandleTrie(HANDLE_HASH_SIZE - 1);
        }
        inline void merge(HandleTrie::TrieValue *other) {
            count += ((Node *) other)->count;
            importance += ((Node *) other)->importance;
        }
        string to_string(); /// String representation of this Node.
    };

    /**
     * Edge object used as the value in HandleTrie.
     */
    class Edge: public HandleTrie::TrieValue {
        public:
        unsigned int count; /// Count for this edge.
        Node *node1; /// Source Node.
        Node *node2; /// Target node.
        Edge() {
            count = 1;
            node1 = node2 = NULL;
        }
        inline void merge(HandleTrie::TrieValue *other) {
            count += ((Edge *) other)->count;
        }
        string to_string(); /// String representation of this Edge.
    };

    /**
     * Adds a new node to this network or increase +1 to its count if it already exists.
     *
     * @param handle Atom being added.
     *
     * @return the value object attached to the node being inserted.
     */
    Node *add_node(string handle);

    /**
     * Adds a new edge handle1->handle2 to this network or increase +1 to its count if it already exists.
     *
     * @param handle1 Source of the edge.
     * @param handle2 Target of the edge.
     *
     * @return the value object attached to the edge being inserted.
     */
    Edge *add_asymmetric_edge(string handle1, string handle2, Node *node1, Node *node2);

    /**
     * Adds new edges handle1->handle2 and handle2->handle1 to this network or increase +1
     * to their count if they already exist.
     *
     * @param handle1 One of the nodes in the edge.
     * @param handle2 The other node in the edge.
     */
    void add_symmetric_edge(string handle1, string handle2, Node *node1, Node *node2);

    /**
     * Lookup and return the value attached to the passed handle.
     *
     * @param handle Handle of the node being searched.
     *
     * @return The value object attached to the node.
     */
    Node *lookup_node(string handle);

    /**
     * Lookup for the passed node and return its count.
     *
     * @param handle Handle of the node being searched.
     *
     * @return The count of the passed node.
     */
    unsigned int get_node_count(string handle);

    /**
     * Lookup for the passed node and return its importance.
     *
     * @param handle Handle of the node being searched.
     *
     * @return The importance of the passed node.
     */
    ImportanceType get_node_importance(string handle);

    /**
     * Lookup for the passed edge and return its count.
     *
     * @param source Source of the edge being searched.
     * @param target Target of the edge being searched.
     *
     * @returnThe count of the passed edge.
     */
    unsigned int get_asymmetric_edge_count(string handle1, string handle2);

    /**
     * Traverse the node's HandleTrie and call the passed function once for each
     * of the visited nodes.
     *
     * @param keep_root_locked True iff HandleTrie root should be kept locked during
     * all the traversal. If false, the root lock is freed just like any other internal
     * trie node.
     * @param visit_function Function to be called passing each visited node. This function
     * @param data Additional data to be passed to the visit_function.
     *
     * expects the Node and a pointer to eventual data used inside visit_function.
     */
    void visit_nodes(
        bool keep_root_locked,
        bool (*visit_function)(HandleTrie::TrieNode *node, void *data),
        void *data);

    ImportanceType alienate_tokens();

private:

    HandleTrie *nodes;
    ImportanceType tokens_to_distribute;
    mutex tokens_mutex;
};

} // namespace attention_broker_server

#endif // _ATTENTION_BROKER_SERVER_HEBBIANNETWORK_H
