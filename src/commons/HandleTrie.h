#pragma once

#include <atomic>
#include <mutex>
#include <string>

#include "Utils.h"

#define TRIE_ALPHABET_SIZE ((unsigned int) 16)

using namespace std;

namespace commons {

/**
 * Data abstraction implementing a map handle->value using a trie (prefix tree).
 *
 * This data structure is basically like a hashmap mapping from handles to objects of
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
        TrieValue();                               /// basic empty constructor.
       public:
        virtual ~TrieValue();                      /// Destructor.
        virtual void merge(TrieValue* other) = 0;  /// Called when a repeated handle is inserted.
        virtual string to_string();  /// Returns a string representation of the value object.
        virtual void* get_stored_object(
            bool clone) {            /// Return the actual object being stored (or a clone of it).
            RAISE_ERROR("get_stored_object() is not implemented for this TrieValue subclass.");
            return NULL;             // Just to avoid warnings
        }
    };

    /**
     * A node in the prefix tree used to store keys.
     */
    class TrieNode {
       public:
        TrieNode();           /// Basic empty constructor.
        ~TrieNode();          /// Destructor.

        TrieNode** children;  /// Array with children of this node.
        TrieValue* value;     /// Value attached to this node or NULL if none.
        string
            suffix;  /// The key (handle) attached to this node (leafs) or NULL if none (internal nodes).
        unsigned char suffix_start;  /// The point in the suffix from which this node (leaf) differs from
                                     /// its siblings.
        mutex trie_node_mutex;

        string to_string();  /// Returns a string representation of this node.
    };

    HandleTrie(unsigned int key_size);  /// Basic constructor.
    ~HandleTrie();                      /// Destructor.

    /**
     * Insert a new key in this HandleTrie or merge its value if the key is already present.
     *
     * @param key Handle being inserted.
     * @param value HandleTrie::TrieValue object being inserted.
     *
     * @return The resulting HandleTrie::TrieValue object after insertion (and eventually the merge) is
     * processed.
     */
    TrieValue* insert(const string& key, TrieValue* value);

    /**
     * Lookup for a given handle.
     *
     * @param key Handle being searched.
     *
     * @return The HandleTrie::TrieValue object attached to the passed key or NULL if none.
     */
    inline TrieValue* lookup(const string& key) { return fetch<TrieValue>(key, true, false); }

    /**
     * Lookup for a given handle and return the corresponding object stored in TrieValue.
     *
     * @param key Handle being searched.
     * @param clone A flag indicating if a clone is supposed to be returned instead of the actual
     * object being stored. It's defaulted to "true". Use clone=false with caution as the actual
     * raw pointer to the inner stored object inside the TrieValue is returned.
     *
     * @return The object actually being stored inside a HandleTrie::TrieValue object attached to the
     * passed key.
     */
    inline void* lookup_stored_object(const string& key, bool clone = true) {
        return fetch<void>(key, true, clone);
    }

    bool exists(const string& key);

    /**
     * Remove a key from this HandleTrie and its associated value.
     *
     * @param key Handle being removed.
     * @param delete_value Whether to delete the value associated with the key.
     *
     * @return true if the key was found and removed, false otherwise.
     */
    bool remove(const string& key, bool delete_value = true);

    /**
     * Traverse all keys (in-order) calling the passed visit_function once per stored value.
     *
     * @param keep_root_locked Keep root HandleTrie::TrieNode locked during the whole traversing
     * releasing the lock when it ends.
     * @param visit_function Function to be called when each value is visited.
     * @param data Additional information passed to visit_function or NULL if none.
     */
    void traverse(bool keep_root_locked, bool (*visit_function)(TrieNode* node, void* data), void* data);

    /**
     * Return the number of elements stored in this HandleTrie.
     *
     * @return The number of elements stored in this HandleTrie.
     */
    inline unsigned int size() { return (unsigned int) this->_size; }

    TrieNode* root;

   private:
    template <typename T>
    T* fetch(const string& key, bool unlock_node, bool clone_stored_object) {
        if (key.size() != key_size) {
            RAISE_ERROR("Invalid key size: " + to_string(key.size()) + " != " + to_string(key_size));
        }

        TrieNode* tree_cursor = root;
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
                TrieNode* node = match ? tree_cursor : NULL;
                T* answer = NULL;
                bool forced_lock_release = false;
                if (node != NULL) {
                    if constexpr (is_same_v<T, TrieNode>) {
                        answer = node;
                    } else if constexpr (is_same_v<T, TrieValue>) {
                        forced_lock_release = true;
                        answer = node->value;
                    } else if constexpr (is_same_v<T, void>) {
                        forced_lock_release = true;
                        if (node->value != NULL) {
                            answer = node->value->get_stored_object(clone_stored_object);
                        }
                    } else {
                        RAISE_ERROR("Invalid fetch() attempt with unexpected type");
                    }
                } else {
                    forced_lock_release = true;
                }
                if (unlock_node || forced_lock_release) {
                    tree_cursor->trie_node_mutex.unlock();
                }
                return answer;
            } else {
                unsigned char c = TLB[(unsigned char) key[key_cursor]];
                TrieNode* child = tree_cursor->children[c];
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
    atomic<unsigned int> _size;
};

/**
 * Dummy TrieValue for using HandleTrie as a set (presence only).
 * Use this when you only need to track which keys exist, with no payload.
 * Safe to pass to insert(); merge() is a no-op.
 */
class EmptyTrieValue : public HandleTrie::TrieValue {
   public:
    void merge(HandleTrie::TrieValue* /*other*/) override {}
};

}  // namespace commons
