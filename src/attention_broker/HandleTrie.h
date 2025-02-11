#ifndef _ATTENTION_BROKER_SERVER_HANDLETRIE_H
#define _ATTENTION_BROKER_SERVER_HANDLETRIE_H

#include <mutex>
#include <string>

#define TRIE_ALPHABET_SIZE ((unsigned int) 16)

using namespace std;

namespace attention_broker_server {

/**
 * Data abstraction implementing a map handle->value using a trie (prefix tree).
 *
 * This data structure is basicaly like a hashmap mapping from handles to objects of
 * type HandleTrie::TrieValue.
 *
 * When a (key, value) pair is inserted and key is already present, the method merge()
 * in value is called passing the newly inserted value.
 */
class HandleTrie {

public:

    /**
     * Virtual basic class to be extended by objects used as "values".
     */
    class TrieValue {
        protected:
            TrieValue(); /// basic empty constructor.
        public:
            virtual ~TrieValue(); /// Destructor.
            virtual void merge(TrieValue *other) = 0; /// Called when a repeated handle is inserted.
            virtual string to_string(); /// Returns a string representation of the value object.

    };

    /**
     * A node in the prefix tree used to store keys.
     */
    class TrieNode {
        public:
            TrieNode(); /// Basic empty constructor.
            ~TrieNode(); /// Destructor.

            TrieNode **children; /// Array with children of this node.
            TrieValue *value; /// Value attached to this node or NULL if none.
            string suffix; /// The key (handle) attached to this node (leafs) or NULL if none (internal nodes).
            unsigned char suffix_start; /// The point in the suffix from which this node (leaf) differs from its siblings.
            mutex trie_node_mutex;

            string to_string(); /// Returns a string representation of this node.
    };

    HandleTrie(unsigned int key_size); /// Basic constructor.
    ~HandleTrie(); /// Destructor.

    /**
     * Insert a new key in this HandleTrie or merge its value if the key is already present.
     *
     * @param key Handle being inserted.
     * @param value HandleTrie::TrieValue object being inserted.
     *
     * @return The resulting HandleTrie::TrieValue object after insertion (and eventually the merge) is processed.
     */
    TrieValue *insert(const string &key, TrieValue *value);

    /**
     * Lookup for a given handle.
     *
     * @param key Handle being searched.
     *
     * @return The HandleTrie::TrieValue object attached to the passed key or NULL if none.
     */
    TrieValue *lookup(const string &key);

    /**
     * Traverse all keys (in-order) calling the passed visit_function once per stored value.
     *
     * @param keep_root_locked Keep root HandleTrie::TrieNode locked during the whole traversing
     * releasing the lock when it ends.
     * @param visit_function Function to be called when each value is visited.
     * @param data Additional information passed to visit_function or NULL if none.
     */
    void traverse(bool keep_root_locked, bool (*visit_function)(TrieNode *node, void *data), void *data);

    TrieNode *root;

private:

    static unsigned char TLB[256];
    static bool TLB_INITIALIZED;
    static void TLB_INIT() {
        TLB[(unsigned char) '0'] = 0;
        TLB[(unsigned char) '1'] = 1;
        TLB[(unsigned char) '2'] = 2;
        TLB[(unsigned char) '3'] = 3;
        TLB[(unsigned char) '4'] = 4;
        TLB[(unsigned char) '5'] = 5;
        TLB[(unsigned char) '6'] = 6;
        TLB[(unsigned char) '7'] = 7;
        TLB[(unsigned char) '8'] = 8;
        TLB[(unsigned char) '9'] = 9;
        TLB[(unsigned char) 'a'] = TLB[(unsigned char) 'A'] = 10;
        TLB[(unsigned char) 'b'] = TLB[(unsigned char) 'B'] = 11;
        TLB[(unsigned char) 'c'] = TLB[(unsigned char) 'C'] = 12;
        TLB[(unsigned char) 'd'] = TLB[(unsigned char) 'D'] = 13;
        TLB[(unsigned char) 'e'] = TLB[(unsigned char) 'E'] = 14;
        TLB[(unsigned char) 'f'] = TLB[(unsigned char) 'F'] = 15;
        TLB_INITIALIZED = true;
    }

    unsigned int key_size;
};

} // namespace attention_broker_server

#endif // _ATTENTION_BROKER_SERVER_HANDLETRIE_H
