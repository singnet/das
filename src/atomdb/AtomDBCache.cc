#include "AtomDBCache.h"

using namespace std;
using namespace atomdb;

// #define CACHE_DEBUG

#ifdef CACHE_DEBUG
#define DEBUG_PRINT(x) cout << x << endl
#else
#define DEBUG_PRINT(x)
#endif

shared_ptr<AtomDBCache> AtomDBCache::instance = nullptr;
mutex AtomDBCache::instance_mutex;

shared_ptr<AtomDBCache> AtomDBCache::get_instance() {
    lock_guard<mutex> lock(instance_mutex);
    if (instance == nullptr) {
        instance = shared_ptr<AtomDBCache>(new AtomDBCache());
    }
    return instance;
}

shared_ptr<atomdb_api_types::AtomDocument> AtomDBCache::get_atom_document(const char* handle) {
    lock_guard<mutex> lock(atom_doc_cache_mutex);
    if (atom_doc_cache.find(handle) != atom_doc_cache.end()) {
        DEBUG_PRINT("AtomDBCache::get_atom_document :: Found in cache " << handle);
        return atom_doc_cache[handle];
    }
    DEBUG_PRINT("AtomDBCache::get_atom_document :: Not found in cache " << handle);
    return nullptr;
}

void AtomDBCache::add_atom_document(const char* handle,
                                    shared_ptr<atomdb_api_types::AtomDocument> document) {
    lock_guard<mutex> lock(atom_doc_cache_mutex);
    atom_doc_cache[handle] = document;
}

shared_ptr<atomdb_api_types::HandleSet> AtomDBCache::query_for_pattern(const char* pattern_handle) {
    lock_guard<mutex> lock(pattern_matching_cache_mutex);
    if (pattern_matching_cache.find(pattern_handle) != pattern_matching_cache.end()) {
        DEBUG_PRINT("AtomDBCache::query_for_pattern :: Found in cache " << pattern_handle);
        return pattern_matching_cache[pattern_handle];
    }
    DEBUG_PRINT("AtomDBCache::query_for_pattern :: Not found in cache " << pattern_handle);
    return nullptr;
}

void AtomDBCache::add_pattern_matching(const char* pattern_handle,
                                       shared_ptr<atomdb_api_types::HandleSet> results) {
    lock_guard<mutex> lock(pattern_matching_cache_mutex);
    pattern_matching_cache[pattern_handle] = results;
}

shared_ptr<atomdb_api_types::HandleList> AtomDBCache::query_for_targets(const char* link_handle) {
    lock_guard<mutex> lock(handle_list_cache_mutex);
    if (handle_list_cache.find(link_handle) != handle_list_cache.end()) {
        DEBUG_PRINT("AtomDBCache::query_for_targets :: Found in cache " << link_handle);
        return handle_list_cache[link_handle];
    }
    DEBUG_PRINT("AtomDBCache::query_for_targets :: Not found in cache " << link_handle);
    return nullptr;
}

void AtomDBCache::add_handle_list(const char* link_handle,
                                  shared_ptr<atomdb_api_types::HandleList> results) {
    lock_guard<mutex> lock(handle_list_cache_mutex);
    handle_list_cache[link_handle] = results;
}
