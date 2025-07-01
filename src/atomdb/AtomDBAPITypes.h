#pragma once

#include <algorithm>
#include <optional>

#include "Link.h"
#include "LinkTemplateInterface.h"
#include "Node.h"
#include "Utils.h"

using namespace std;
using namespace commons;

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
};

class AtomDocument {
   public:
    AtomDocument() {}
    virtual ~AtomDocument() {}

    virtual const char* get(const string& key) = 0;
    virtual const char* get(const string& array_key, unsigned int index) = 0;
    virtual unsigned int get_size(const string& array_key) = 0;
    virtual bool contains(const string& key) = 0;
};

}  // namespace atomdb_api_types
}  // namespace atomdb
