#pragma once

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Link.h"
#include "LinkSchema.h"
#include "Node.h"
#include "Utils.h"

using namespace std;
using namespace commons;
using namespace atoms;

namespace atomdb {
namespace atomdb_api_types {

// -------------------------------------------------------------------------------------------------
// NOTE TO REVIEWER:
//
// This class will be replaced/integrated by/with classes already implemented in das-atom-db.
//
// However, that classes will need to be revisited in order to allow the methods implemented here
// because although the design of such methods is nasty, they have the string advantage of
// allowing the reuse of structures allocated by the DBMS (Redis an MongoDB) without the need
// of re-allocation of other dataclasses. Although this nasty behavior may not be desirable
// outside the DAS bounds, it's quite appealing inside the query engine (and perhaps other
// parts of internal stuff).
//
// I think it's pointless to make any further documentation while we don't make this integration.
// -------------------------------------------------------------------------------------------------

class HandleList {
   public:
    HandleList() {}
    virtual ~HandleList() {}

    virtual const char* get_handle(unsigned int index) = 0;
    virtual unsigned int size() = 0;
};

class HandleSetIterator {
   public:
    virtual char* next() = 0;
};

class HandleSet {
   public:
    HandleSet() {}
    virtual ~HandleSet() {}

    virtual unsigned int size() = 0;
    virtual void append(shared_ptr<HandleSet> other) = 0;
    virtual shared_ptr<HandleSetIterator> get_iterator() = 0;

    virtual map<string, string> get_metta_expressions_by_handle(const string& handle) = 0;
    virtual Assignment get_assignments_by_handle(const string& handle) = 0;
};

class AtomDocument {
   public:
    AtomDocument() {}
    virtual ~AtomDocument() {}

    virtual const char* get(const string& key) = 0;
    virtual const char* get(const string& array_key, unsigned int index) = 0;
    virtual bool get_bool(const string& key) = 0;
    virtual unsigned int get_size(const string& array_key) = 0;
    virtual bool contains(const string& key) = 0;
};

// Simple DTO (schema + read/write); concrete in this header because it needs no specific implementation.
class AccessPermissionEntry {
   public:
    AccessPermissionEntry(const LinkSchema& schema, bool read, bool write)
        : schema_(schema), read_(read), write_(write) {}

    AccessPermissionEntry(const vector<string>& tokens, bool read, bool write)
        : schema_(tokens), read_(read), write_(write) {}

    const LinkSchema& schema() const { return this->schema_; }
    bool read() const { return this->read_; }
    bool write() const { return this->write_; }

   private:
    LinkSchema schema_;
    bool read_;
    bool write_;
};

// Simple DTO (public_key + full_access + entries); concrete in this header like AccessPermissionEntry.
class AccessPermissionDocument {
   public:
    AccessPermissionDocument(const string& public_key,
                             bool full_access,
                             const vector<AccessPermissionEntry>& entries)
        : public_key_(public_key), full_access_(full_access), entries_(entries) {}

    const string& public_key() const { return this->public_key_; }
    bool full_access() const { return this->full_access_; }
    const vector<AccessPermissionEntry>& entries() const { return this->entries_; }

   private:
    string public_key_;
    bool full_access_;
    vector<AccessPermissionEntry> entries_;
};

}  // namespace atomdb_api_types
}  // namespace atomdb
