#include "QueryElementRegistry.h"

namespace query_element {

QueryElementRegistry::QueryElementRegistry() {}

QueryElementRegistry::~QueryElementRegistry() {}

void QueryElementRegistry::add(const string& key, shared_ptr<QueryElement> element) {
    lock_guard<mutex> guard(registry_mutex);
    if (registry.find(key) == registry.end()) {
        LOG_DEBUG("QueryElementRegistry::add - added key: " << key);
        registry[key] = element;
    } else {
        Utils::error("Query element with key '" + key + "' already exists in the registry");
    }
}

void QueryElementRegistry::add(const shared_ptr<char>& key, shared_ptr<QueryElement> element) {
    this->add(string((char*) key.get()), element);
}

shared_ptr<QueryElement> QueryElementRegistry::get(const string& key) {
    lock_guard<mutex> guard(registry_mutex);
    auto it = registry.find(key);
    if (it != registry.end()) {
        LOG_DEBUG("QueryElementRegistry::get - found key: " << key);
        return it->second;
    }
    LOG_DEBUG("QueryElementRegistry::get - did not find key: " << key);
    return nullptr;
}

shared_ptr<QueryElement> QueryElementRegistry::get(const shared_ptr<char>& key) {
    return this->get(string((char*) key.get()));
}

}  // namespace query_element
