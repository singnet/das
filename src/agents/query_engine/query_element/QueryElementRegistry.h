#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

// clang-format off
#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"
// clang-format on

#include "QueryElement.h"

using namespace std;

namespace query_element {

/**
 * @brief Class that stores and retrieves QueryElements by their keys.
 *
 * The class allows to store and retrieve QueryElements by their keys, similar to a cache.
 * It is used by the PatternMatchingQueryProcessor to store and retrieve reusable QueryElements
 * created from the query tokens.
 *
 * @note This class is not thread-safe.
 */
class QueryElementRegistry {
   public:
    /**
     * @brief Constructor.
     */
    QueryElementRegistry();

    /**
     * @brief Destructor.
     */
    ~QueryElementRegistry();

    /**
     * @brief Adds a QueryElement to the registry.
     *
     * @param key The key to use to store the QueryElement.
     * @param element The QueryElement to store.
     */
    void add(const string& key, shared_ptr<QueryElement> element);

    /**
     * @brief Alias for add(string, QueryElement).
     */
    void add(const shared_ptr<char>& key, shared_ptr<QueryElement> element);

    /**
     * @brief Retrieves a QueryElement from the registry.
     *
     * @param key The key to use to retrieve the QueryElement.
     * @return The QueryElement associated with the key, or nullptr if none was found.
     */
    shared_ptr<QueryElement> get(const string& key);

    /**
     * @brief Alias for get(string).
     */
    shared_ptr<QueryElement> get(const shared_ptr<char>& key);

   private:
    unordered_map<string, shared_ptr<QueryElement>> registry;
    mutex registry_mutex;
};

}  // namespace query_element
