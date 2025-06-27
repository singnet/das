#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "AtomDBAPITypes.h"

using namespace std;

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
     * @brief Get an atom document from the cache.
     *
     * @param handle The handle of the atom document.
     * @return The atom document if it is cached, nullptr otherwise.
     */
    GetAtomDocumentResult get_atom_document(const string& handle);

    /**
     * @brief Add an atom document to the cache.
     *
     * @param handle The handle of the atom document.
     * @param document The atom document to be cached.
     */
    void add_atom_document(const string& handle, shared_ptr<atomdb_api_types::AtomDocument> document);

    /**
     * @brief Query for a pattern matching.
     *
     * @param link_template A LinkTemplateInterface.
     * @return The result of the query if it is cached, nullptr otherwise.
     */
    QueryForPatternResult query_for_pattern(const LinkTemplateInterface& link_template);

    /**
     * @brief Add a pattern matching to the cache.
     *
     * @param pattern_handle The handle of the pattern.
     * @param results The result of the query to be cached.
     */
    void add_pattern_matching(const string& pattern_handle,
                              shared_ptr<atomdb_api_types::HandleSet> results);

    /**
     * @brief Query for targets.
     *
     * @param link_handle The handle of the link.
     * @return The list of targets if it is cached, nullptr otherwise.
     */
    QueryForTargetsResult query_for_targets(const string& link_handle);

    /**
     * @brief Add a handle list to the cache.
     *
     * @param link_handle The handle of the link.
     * @param results The result of the query to be cached.
     */
    void add_handle_list(const string& link_handle, shared_ptr<atomdb_api_types::HandleList> results);

   private:
    /**
     * @brief The cache for atom documents.
     */
    unordered_map<string, shared_ptr<atomdb_api_types::AtomDocument>> atom_doc_cache;
    /**
     * @brief The mutex for the atom document cache.
     */
    mutex atom_doc_cache_mutex;

    /**
     * @brief The cache for pattern matching results.
     */
    unordered_map<string, shared_ptr<atomdb_api_types::HandleSet>> pattern_matching_cache;
    /**
     * @brief The mutex for the pattern matching cache.
     */
    mutex pattern_matching_cache_mutex;

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
