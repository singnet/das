#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#define LOG_LEVEL DEBUG_LEVEL

#include "Logger.h"
#include "QueryElement.h"

using namespace std;

namespace query_element {

class QueryElementRegistry {
   public:
    QueryElementRegistry() {}
    ~QueryElementRegistry() {}

    void add(const shared_ptr<char>& key, shared_ptr<QueryElement> element) {
        this->add(string((char*) key.get()), element);
    }

    void add(const string& key, shared_ptr<QueryElement> element) {
        lock_guard<mutex> guard(this->registry_mutex);
        if (this->registry.find(key) == this->registry.end()) {
            this->registry[key] = element;
        } else {
            Utils::error("Query element with key '" + key + "' already exists in the registry");
        }
    }

    shared_ptr<QueryElement> get(const shared_ptr<char>& key) {
        return this->get(string((char*) key.get()));
    }

    shared_ptr<QueryElement> get(const string& key) {
        lock_guard<mutex> guard(this->registry_mutex);
        auto it = this->registry.find(key);
        if (it != this->registry.end()) {
            LOG_DEBUG("QueryElementRegistry::get - found key: " << key);
            return it->second;
        }
        LOG_DEBUG("QueryElementRegistry::get - did not find key: " << key);
        return nullptr;
    }

   private:
    unordered_map<string, shared_ptr<QueryElement>> registry;
    mutex registry_mutex;
};

}  // namespace query_element
