#include "AtomDBCache.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace atomdb;

AtomDBCache::GetAtomDocumentResult AtomDBCache::get_node_document(const string& handle) {
    lock_guard<mutex> lock(node_doc_cache_mutex);
    if (node_doc_cache.find(handle) != node_doc_cache.end()) {
        LOG_DEBUG("cache hit " << handle);
        return {true, node_doc_cache[handle]};
    }
    LOG_DEBUG("cache miss " << handle);
    return {false, nullptr};
}

void AtomDBCache::add_node_document(const string& handle,
                                    shared_ptr<atomdb_api_types::AtomDocument> document) {
    lock_guard<mutex> lock(node_doc_cache_mutex);
    node_doc_cache[handle] = document;
}

void AtomDBCache::erase_node_document_cache(const string& handle) {
    lock_guard<mutex> lock(node_doc_cache_mutex);
    node_doc_cache.erase(handle);
}

AtomDBCache::GetAtomDocumentResult AtomDBCache::get_link_document(const string& handle) {
    lock_guard<mutex> lock(link_doc_cache_mutex);
    if (link_doc_cache.find(handle) != link_doc_cache.end()) {
        LOG_DEBUG("cache hit " << handle);
        return {true, link_doc_cache[handle]};
    }
    LOG_DEBUG("cache miss " << handle);
    return {false, nullptr};
}

void AtomDBCache::add_link_document(const string& handle,
                                    shared_ptr<atomdb_api_types::AtomDocument> document) {
    lock_guard<mutex> lock(link_doc_cache_mutex);
    link_doc_cache[handle] = document;
}

void AtomDBCache::erase_link_document_cache(const string& handle) {
    lock_guard<mutex> lock(link_doc_cache_mutex);
    link_doc_cache.erase(handle);
}

AtomDBCache::QueryForPatternResult AtomDBCache::query_for_pattern(const string& pattern_handle) {
    lock_guard<mutex> lock(pattern_matching_cache_mutex);
    if (pattern_matching_cache.find(pattern_handle) != pattern_matching_cache.end()) {
        LOG_DEBUG("cache hit " << pattern_handle);
        return {true, pattern_matching_cache[pattern_handle]};
    }
    LOG_DEBUG("cache miss " << pattern_handle);
    return {false, nullptr};
}

void AtomDBCache::add_pattern_matching(const string& pattern_handle,
                                       shared_ptr<atomdb_api_types::HandleSet> results) {
    lock_guard<mutex> lock(pattern_matching_cache_mutex);
    pattern_matching_cache[pattern_handle] = results;
}

void AtomDBCache::erase_pattern_matching_cache(const string& pattern_handle) {
    lock_guard<mutex> lock(pattern_matching_cache_mutex);
    pattern_matching_cache.erase(pattern_handle);
}

void AtomDBCache::clear_all_pattern_handles() {
    lock_guard<mutex> lock(pattern_matching_cache_mutex);
    pattern_matching_cache.clear();
}

AtomDBCache::QueryForTargetsResult AtomDBCache::query_for_targets(const string& link_handle) {
    lock_guard<mutex> lock(handle_list_cache_mutex);
    if (handle_list_cache.find(link_handle) != handle_list_cache.end()) {
        LOG_DEBUG("cache hit " << link_handle);
        return {true, handle_list_cache[link_handle]};
    }
    LOG_DEBUG("cache miss " << link_handle);
    return {false, nullptr};
}

void AtomDBCache::add_handle_targets(const string& link_handle,
                                     shared_ptr<atomdb_api_types::HandleList> results) {
    lock_guard<mutex> lock(handle_list_cache_mutex);
    handle_list_cache[link_handle] = results;
}

void AtomDBCache::erase_handle_targets_cache(const string& link_handle) {
    lock_guard<mutex> lock(handle_list_cache_mutex);
    cout << "XXX erase_handle_targets_cache " << link_handle << endl;
    handle_list_cache.erase(link_handle);
}

void AtomDBCache::clear_all_targets_handles() {
    lock_guard<mutex> lock(handle_list_cache_mutex);
    handle_list_cache.clear();
}

AtomDBCache::QueryForIncomingResult AtomDBCache::query_for_incoming_set(const string& handle) {
    lock_guard<mutex> lock(incoming_set_cache_mutex);
    if (incoming_set_cache.find(handle) != incoming_set_cache.end()) {
        LOG_DEBUG("cache hit " << handle);
        return {true, incoming_set_cache[handle]};
    }
    LOG_DEBUG("cache miss " << handle);
    return {false, nullptr};
}

void AtomDBCache::add_incoming_set(const string& handle,
                                   shared_ptr<atomdb_api_types::HandleSet> results) {
    lock_guard<mutex> lock(incoming_set_cache_mutex);
    incoming_set_cache[handle] = results;
}

void AtomDBCache::erase_incoming_set_cache(const string& handle) {
    lock_guard<mutex> lock(incoming_set_cache_mutex);
    incoming_set_cache.erase(handle);
}

void AtomDBCache::clear_all_incoming_handles() {
    lock_guard<mutex> lock(incoming_set_cache_mutex);
    incoming_set_cache.clear();
}
