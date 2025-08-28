#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "AtomDBAPITypes.h"
#include "LinkSchema.h"

using namespace std;
using namespace atoms;

namespace atomdb {

/**
 * @brief A cache for results from the AtomDB.
 *
 * This class is a simple cache for results from the AtomDB. It stores the results in memory
 * and returns them directly if they are already cached.
 *
 * @note This class can be instantiated as a singleton with the AtomDBCacheSingleton class.
 */
class AtomDBCache {
   public:
    /**
     * @brief The result type for cached pattern matching.
     */
    typedef struct {
        bool is_cache_hit;
        shared_ptr<atomdb_api_types::HandleSet> result;
    } QueryForPatternResult;

    /**
     * @brief The result type for cached atom documents.
     */
    typedef struct {
        bool is_cache_hit;
        shared_ptr<atomdb_api_types::AtomDocument> result;
    } GetAtomDocumentResult;

    /**
     * @brief The result type for cached targets.
     */
    typedef struct {
        bool is_cache_hit;
        shared_ptr<atomdb_api_types::HandleList> result;
    } QueryForTargetsResult;

    /**
     * @brief The result type for cached incoming set values.
     */
    typedef struct {
        bool is_cache_hit;
        shared_ptr<atomdb_api_types::HandleSet> result;
    } QueryForIncomingResult;

    /**
     * @brief Constructor.
     *
     * The constructor is private to ensure that the correct instance is created.
     */
    AtomDBCache() {}

    /**
     * @brief Destructor.
     */
    virtual ~AtomDBCache() {}

    /**
     * @brief Get a node document from the cache.
     *
     * @param handle The handle of the node document.
     * @return The node document if it is cached, nullptr otherwise.
     */
    GetAtomDocumentResult get_node_document(const string& handle);

    /**
     * @brief Add a node document to the cache.
     *
     * @param handle The handle of the node document.
     * @param document The node document to be cached.
     */
    void add_node_document(const string& handle, shared_ptr<atomdb_api_types::AtomDocument> document);

    /**
     * @brief Invalidate the node document cache for a handle.
     *
     * @param handle The handle of the node document.
     */
    void erase_node_document_cache(const string& handle);

    /**
     * @brief Get a link document from the cache.
     *
     * @param handle The handle of the link document.
     * @return The link document if it is cached, nullptr otherwise.
     */
    GetAtomDocumentResult get_link_document(const string& handle);

    /**
     * @brief Add a link document to the cache.
     *
     * @param handle The handle of the link document.
     * @param document The link document to be cached.
     */
    void add_link_document(const string& handle, shared_ptr<atomdb_api_types::AtomDocument> document);

    /**
     * @brief Invalidate the link document cache for a handle.
     *
     * @param handle The handle of the link document.
     */
    void erase_link_document_cache(const string& handle);

    /**
     * @brief Query for a pattern matching.
     *
     * @param pattern_handle The handle of the pattern.
     * @return The result of the query if it is cached, nullptr otherwise.
     */
    QueryForPatternResult query_for_pattern(const string& pattern_handle);

    /**
     * @brief Add a pattern matching to the cache.
     *
     * @param pattern_handle The handle of the pattern.
     * @param results The result of the query to be cached.
     */
    void add_pattern_matching(const string& pattern_handle,
                              shared_ptr<atomdb_api_types::HandleSet> results);

    /**
     * @brief Invalidate the pattern matching cache for a handle.
     *
     * @param pattern_handle The handle of the pattern.
     */
    void erase_pattern_matching_cache(const string& pattern_handle);

    /**
     * @brief Clear the pattern matching cache for all handles.
     */
    void clear_all_pattern_handles();

    /**
     * @brief Query for targets.
     *
     * @param link_handle The handle of the link.
     * @return The list of targets if it is cached, nullptr otherwise.
     */
    QueryForTargetsResult query_for_targets(const string& link_handle);

    /**
     * @brief Add handle's targets to the cache.
     *
     * @param link_handle The handle of the link.
     * @param results The result of the query to be cached.
     */
    void add_handle_targets(const string& link_handle, shared_ptr<atomdb_api_types::HandleList> results);

    /**
     * @brief Invalidate the targets cache for a handle.
     *
     * @param link_handle The handle of the link.
     */
    void erase_handle_targets_cache(const string& link_handle);

    /**
     * @brief Clear all targets cache.
     */
    void clear_all_targets_handles();

    /**
     * @brief Query for incoming set values.
     *
     * @param handle The handle of the atom.
     * @return The list of incoming handles if it is cached, nullptr otherwise.
     */
    QueryForIncomingResult query_for_incoming_set(const string& handle);

    /**
     * @brief Add an incoming set to the cache.
     *
     * @param handle The handle of the atom.
     * @param results The result of the query to be cached.
     */
    void add_incoming_set(const string& handle, shared_ptr<atomdb_api_types::HandleSet> results);

    /**
     * @brief Invalidate the incoming set cache for a handle.
     *
     * @param handle The handle of the atom.
     */
    void erase_incoming_set_cache(const string& handle);

    /**
     * @brief Clear all incoming set cache.
     */
    void clear_all_incoming_handles();

   private:
    /**
     * @brief The cache for node documents.
     */
    unordered_map<string, shared_ptr<atomdb_api_types::AtomDocument>> node_doc_cache;
    /**
     * @brief The mutex for the node document cache.
     */
    mutex node_doc_cache_mutex;

    /**
     * @brief The cache for link documents.
     */
    unordered_map<string, shared_ptr<atomdb_api_types::AtomDocument>> link_doc_cache;
    /**
     * @brief The mutex for the link document cache.
     */
    mutex link_doc_cache_mutex;

    /**
     * @brief The cache for pattern matching results.
     */
    unordered_map<string, shared_ptr<atomdb_api_types::HandleSet>> pattern_matching_cache;
    /**
     * @brief The mutex for the pattern matching cache.
     */
    mutex pattern_matching_cache_mutex;

    /**
     * @brief The cache for incoming set values.
     */
    unordered_map<string, shared_ptr<atomdb_api_types::HandleSet>> incoming_set_cache;
    /**
     * @brief The mutex for the incoming set cache.
     */
    mutex incoming_set_cache_mutex;

    /**
     * @brief The cache for target handle lists.
     */
    unordered_map<string, shared_ptr<atomdb_api_types::HandleList>> handle_list_cache;
    /**
     * @brief The mutex for the target handle list cache.
     */
    mutex handle_list_cache_mutex;
};

}  // namespace atomdb
