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
    HandleSetMork(string handle, map<string, string> metta_expressions, Assignment assignments);
    ~HandleSetMork();

    unsigned int size();
    void append(shared_ptr<HandleSet> other);
    shared_ptr<HandleSetIterator> get_iterator();

    unsigned int handles_size;
    unordered_set<string> handles;
    map<string, map<string, string>> metta_expressions_by_handle;
    map<string, Assignment> assignments_by_handle;
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

class HandleListMork : public HandleList {
   public:
    HandleListMork(vector<string> targets);
    ~HandleListMork();

    const char* get_handle(unsigned int index);
    unsigned int size();

   private:
    unsigned int handles_size;
    vector<string> handles;
};

}  // namespace atomdb_api_types

}  // namespace atomdb
