#include "AtomDBCache.h"

#define LOG_LEVEL INFO_LEVEL

#include "Logger.h"

using namespace std;
using namespace atomdb;

AtomDBCache::GetAtomDocumentResult AtomDBCache::get_atom_document(const char* handle) {
    lock_guard<mutex> lock(atom_doc_cache_mutex);
    if (atom_doc_cache.find(handle) != atom_doc_cache.end()) {
        LOG_DEBUG("cache hit " << handle);
        return {true, atom_doc_cache[handle]};
    }
    LOG_DEBUG("cache miss " << handle);
    return {false, nullptr};
}

void AtomDBCache::add_atom_document(const char* handle,
                                    shared_ptr<atomdb_api_types::AtomDocument> document) {
    lock_guard<mutex> lock(atom_doc_cache_mutex);
    atom_doc_cache[handle] = document;
}

AtomDBCache::QueryForPatternResult AtomDBCache::query_for_pattern(const char* pattern_handle) {
    lock_guard<mutex> lock(pattern_matching_cache_mutex);
    if (pattern_matching_cache.find(pattern_handle) != pattern_matching_cache.end()) {
        LOG_DEBUG("cache hit " << pattern_handle);
        return {true, pattern_matching_cache[pattern_handle]};
    }
    LOG_DEBUG("cache miss " << pattern_handle);
    return {false, nullptr};
}

void AtomDBCache::add_pattern_matching(const char* pattern_handle,
                                       shared_ptr<atomdb_api_types::HandleSet> results) {
    lock_guard<mutex> lock(pattern_matching_cache_mutex);
    pattern_matching_cache[pattern_handle] = results;
}

AtomDBCache::QueryForTargetsResult AtomDBCache::query_for_targets(const char* link_handle) {
    lock_guard<mutex> lock(handle_list_cache_mutex);
    if (handle_list_cache.find(link_handle) != handle_list_cache.end()) {
        LOG_DEBUG("cache hit " << link_handle);
        return {true, handle_list_cache[link_handle]};
    }
    LOG_DEBUG("cache miss " << link_handle);
    return {false, nullptr};
}

void AtomDBCache::add_handle_list(const char* link_handle,
                                  shared_ptr<atomdb_api_types::HandleList> results) {
    lock_guard<mutex> lock(handle_list_cache_mutex);
    handle_list_cache[link_handle] = results;
}
