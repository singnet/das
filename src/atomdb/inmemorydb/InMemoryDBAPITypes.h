#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "Assignment.h"
#include "AtomDBAPITypes.h"

using namespace std;
using namespace commons;

namespace atomdb {
namespace atomdb_api_types {

class HandleSetInMemory : public HandleSet {
    friend class HandleSetInMemoryIterator;

   public:
    HandleSetInMemory();
    ~HandleSetInMemory();

    unsigned int size() override;
    void append(shared_ptr<HandleSet> other) override;
    shared_ptr<HandleSetIterator> get_iterator() override;

    map<string, string> get_metta_expressions_by_handle(const string& handle) override;
    Assignment get_assignments_by_handle(const string& handle) override;

    void add_handle(const string& handle);

   private:
    set<string> handles;
    map<string, map<string, string>> metta_expressions_by_handle;
    map<string, Assignment> assignments_by_handle;
};

class HandleSetInMemoryIterator : public HandleSetIterator {
   public:
    HandleSetInMemoryIterator(HandleSetInMemory* handle_set);
    ~HandleSetInMemoryIterator();

    char* next() override;

   private:
    HandleSetInMemory* handle_set;
    set<string>::iterator it;
    vector<char*> allocated_strings;
};

class HandleListInMemory : public HandleList {
   public:
    HandleListInMemory();
    HandleListInMemory(const vector<string>& handles);
    ~HandleListInMemory();

    const char* get_handle(unsigned int index) override;
    unsigned int size() override;

    void add_handle(const string& handle);

   private:
    vector<string> handles;
    vector<char*> allocated_strings;
};

}  // namespace atomdb_api_types
}  // namespace atomdb
