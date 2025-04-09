#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "AtomDBSingleton.h"

using namespace std;

namespace query_element {

/**
 * @brief Singleton class to cache LinkTemplate query results.
 *
 * The PatternMatchingCache is a singleton that caches the query results of LinkTemplates.
 * This is beneficial because LinkTemplates can be expensive to execute, and the same
 * LinkTemplate can be reused multiple times in the same query or in different queries.
 *
 * @see LinkTemplate
 */
class PatternMatchingCache {
    public:
     ~PatternMatchingCache() {}

     /**
      * Get the instance of the PatternMatchingCache.
      * @return A shared pointer to the instance of the PatternMatchingCache.
      */
     static shared_ptr<PatternMatchingCache> get_instance();

     /**
      * Get the cached query result for the given LinkTemplate handle.
      * @param handle Handle of the LinkTemplate.
      * @return A vector of strings representing the query result.
      */
     const vector<string>& get(const shared_ptr<char>& handle);

    private:
     PatternMatchingCache() : atom_db(atomdb::AtomDBSingleton::get_instance()) {}

     static shared_ptr<PatternMatchingCache> cache_instance;
     static mutex cache_instance_mutex;

     shared_ptr<atomdb::AtomDB> atom_db;
     unordered_map<string, vector<string>> pattern_matching_cache;
     mutex pattern_matching_cache_mutex;
};

}  // namespace query_element
