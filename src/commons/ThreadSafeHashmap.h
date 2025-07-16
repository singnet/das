#pragma once

#include <mutex>
#include <unordered_map>

#include "Utils.h"

namespace commons {

template <typename KEY_TYPE, typename VALUE_TYPE>
class ThreadSafeHashmap {
   private:
    unordered_map<KEY_TYPE, VALUE_TYPE> table;
    mutex api_mutex;

   public:
    ThreadSafeHashmap() { lock_guard<mutex> semaphore(this->api_mutex); }

    ~ThreadSafeHashmap() { lock_guard<mutex> semaphore(this->api_mutex); }

    void set(const KEY_TYPE& key, const VALUE_TYPE& value) {
        lock_guard<mutex> semaphore(this->api_mutex);
        this->table[key] = value;
    }

    const VALUE_TYPE& get(const KEY_TYPE& key) {
        lock_guard<mutex> semaphore(this->api_mutex);
        auto iterator = this->table.find(key);
        if (iterator == this->table.end()) {
            Utils::error("Lookup for non-existent key in ThreadSafeHashmap");
        }
        return iterator->second;
    }

    bool contains(const KEY_TYPE& key) {
        lock_guard<mutex> semaphore(this->api_mutex);
        return (this->table.find(key) != this->table.end());
    }

    void clear() {
        lock_guard<mutex> semaphore(this->api_mutex);
        this->table.clear();
    }
};
}  // namespace commons
