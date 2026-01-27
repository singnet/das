#include "InMemoryDBAPITypes.h"

#include <cstring>

#include "Utils.h"

using namespace atomdb;
using namespace atomdb_api_types;
using namespace commons;
using namespace std;

// HandleSetInMemory
HandleSetInMemory::HandleSetInMemory() : HandleSet() {}

HandleSetInMemory::~HandleSetInMemory() {}

unsigned int HandleSetInMemory::size() { return handles.size(); }

void HandleSetInMemory::append(shared_ptr<HandleSet> other) {
    auto handle_set_inmemory = dynamic_pointer_cast<HandleSetInMemory>(other);
    if (handle_set_inmemory) {
        for (const auto& handle : handle_set_inmemory->handles) {
            handles.insert(handle);
        }
        // Merge metta expressions and assignments
        for (const auto& [handle, exprs] : handle_set_inmemory->metta_expressions_by_handle) {
            metta_expressions_by_handle[handle] = exprs;
        }
        for (const auto& [handle, assignment] : handle_set_inmemory->assignments_by_handle) {
            assignments_by_handle[handle] = assignment;
        }
    }
}

shared_ptr<HandleSetIterator> HandleSetInMemory::get_iterator() {
    shared_ptr<HandleSetInMemoryIterator> it(new HandleSetInMemoryIterator(this));
    return it;
}

map<string, string> HandleSetInMemory::get_metta_expressions_by_handle(const string& handle) {
    auto it = metta_expressions_by_handle.find(handle);
    if (it != metta_expressions_by_handle.end()) {
        return it->second;
    }
    return {};
}

Assignment HandleSetInMemory::get_assignments_by_handle(const string& handle) {
    auto it = assignments_by_handle.find(handle);
    if (it != assignments_by_handle.end()) {
        return it->second;
    }
    return Assignment();
}

void HandleSetInMemory::add_handle(const string& handle) { handles.insert(handle); }

// HandleSetInMemoryIterator
HandleSetInMemoryIterator::HandleSetInMemoryIterator(HandleSetInMemory* handle_set)
    : handle_set(handle_set), it(handle_set->handles.begin()) {}

HandleSetInMemoryIterator::~HandleSetInMemoryIterator() {
    for (auto ptr : allocated_strings) {
        delete[] ptr;
    }
}

char* HandleSetInMemoryIterator::next() {
    if (it == handle_set->handles.end()) {
        return nullptr;
    }
    string handle = *it;
    ++it;
    char* handle_cstr = new char[handle.size() + 1];
    strcpy(handle_cstr, handle.c_str());
    allocated_strings.push_back(handle_cstr);
    return handle_cstr;
}

// HandleListInMemory
HandleListInMemory::HandleListInMemory() : HandleList() {}

HandleListInMemory::HandleListInMemory(const vector<string>& handles) : HandleList(), handles(handles) {
    for (const auto& handle : handles) {
        char* handle_cstr = new char[handle.size() + 1];
        strcpy(handle_cstr, handle.c_str());
        allocated_strings.push_back(handle_cstr);
    }
}

HandleListInMemory::~HandleListInMemory() {
    for (auto ptr : allocated_strings) {
        delete[] ptr;
    }
}

const char* HandleListInMemory::get_handle(unsigned int index) {
    if (index >= handles.size()) {
        Utils::error("Handle index out of bounds: " + to_string(index) +
                     " Answer handles size: " + to_string(handles.size()));
    }
    return allocated_strings[index];
}

unsigned int HandleListInMemory::size() { return handles.size(); }

void HandleListInMemory::add_handle(const string& handle) {
    handles.push_back(handle);
    char* handle_cstr = new char[handle.size() + 1];
    strcpy(handle_cstr, handle.c_str());
    allocated_strings.push_back(handle_cstr);
}
