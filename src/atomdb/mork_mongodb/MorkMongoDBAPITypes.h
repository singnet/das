#pragma once

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/json.hpp>
#include <memory>
#include <mongocxx/client.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <unordered_set>
#include <vector>

#include "AtomDBAPITypes.h"
#include "Utils.h"

namespace atomdb {

namespace atomdb_api_types {

class HandleSetMork : public HandleSet {
    friend class HandleSetMorkIterator;

   public:
    HandleSetMork();
    HandleSetMork(string handle);
    ~HandleSetMork();

    unsigned int size();
    void append(shared_ptr<HandleSet> other);
    shared_ptr<HandleSetIterator> get_iterator();

   private:
    unsigned int handles_size;
    unordered_set<string> handles;
};

class HandleSetMorkIterator : public HandleSetIterator {
   public:
    HandleSetMorkIterator(HandleSetMork* handle_set);
    ~HandleSetMorkIterator();

    char* next();

   private:
    HandleSetMork* handle_set;
    unordered_set<string>::iterator it;
    unordered_set<string>::iterator end;
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
