#include "PatternMatchingCache.h"

#include "Utils.h"

using namespace query_element;

shared_ptr<PatternMatchingCache> PatternMatchingCache::cache_instance = nullptr;
mutex PatternMatchingCache::cache_instance_mutex;

#define DEBUG

#ifdef DEBUG
#define COUT(x) cout << x << endl
#else
#define COUT(x)
#endif

// --------------------------------------------------------------------------------
// Public methods

shared_ptr<PatternMatchingCache> PatternMatchingCache::get_instance() {
    lock_guard<mutex> l(cache_instance_mutex);
    if (cache_instance == nullptr) {
        cache_instance = shared_ptr<PatternMatchingCache>(new PatternMatchingCache());
    }
    return cache_instance;
}

const vector<string>& PatternMatchingCache::get(const shared_ptr<char>& handle) {
    lock_guard<mutex> l(this->pattern_matching_cache_mutex);
    string handle_str((char*) handle.get());
    if (this->pattern_matching_cache.find(handle_str) == this->pattern_matching_cache.end()) {
        COUT("PatternMatchingCache::get() - CACHE MISS - handle: " << handle_str);
        this->pattern_matching_cache[handle_str] = move(this->atom_db->query_for_pattern(handle));
    } else {
        COUT("PatternMatchingCache::get() - CACHE HIT - handle: " << handle_str);
    }
    return this->pattern_matching_cache[handle_str];
}