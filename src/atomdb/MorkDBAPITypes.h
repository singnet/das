#pragma once

#include <vector>
#include <memory>
#include <unordered_set>

#include "AtomDBAPITypes.h"
#include "Utils.h"

namespace atomdb {

namespace atomdb_api_types {

class HandleSetMork : public HandleSet {
    public:
        HandleSetMork();
        HandleSetMork(std::string handle);
        ~HandleSetMork();

        unsigned int size();
        void append(std::shared_ptr<HandleSet> other);
        std::shared_ptr<HandleSetIterator> get_iterator();

    private:
        unsigned int handles_size;
        std::unordered_set<std::string> handles;
};

class HandleSetMorkIterator : public HandleSetIterator {
    public:
        HandleSetMorkIterator(HandleSetMork* handle_set);
        ~HandleSetMorkIterator();

        char* next();

    private:
        HandleSetMork* handle_set;
        std::string current;
};

// WIP
class HandleListMork : public HandleList {
    public:
        HandleListMork();
        ~HandleListMork();

        const char* get_handle(unsigned int index);
        unsigned int size();
};  

}  // namespace atomdb_api_types

}  // namespace atomdb

