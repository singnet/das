/*

### Methods

```
// scope is a value in {REMOTE_ONLY, LOCAL_ONLY, LOCAL_AND_REMOTE}
// All scope parameters below should be defaulted to LOCAL_AND_REMOTE

// handle is a char*
// I'm not sure how to store *targets list*

* const Atom& get_atom(handle, scope)
* const Link& get_link(type, *targets list*, scope)
* const Node& get_node(type, name, scope)
* shared_ptr<PatternMatchingQueryProxy> pattern_matching_query(query, answers_count = 0) // answers_count
== 0 indicates ALL answers
* unsigned int pattern_matching_count(query) // query is NULL or empty indicates count ALL atoms in the
knowledge base
* void pattern_matching_fetch(query, answers_count = 0) // answers_count == 0 indicates ALL answers
* char*  add_node(type, name) // return handle
* char*  add_link(type, *targets list*) // return handle
* void commit_changes(scope)
```

### State

```
* shared_ptr<AtomDB> db
* shared_ptr<ServiceBus> bus
* // a wrapper for a HandleTrie which maps from handle -> atom
```

### Methods behavior

```
* get_atom(handle)
* get_link(type, *targets list*)
* get_node(type, name)
```

LOCAL_ONLY and REMOTE_ONLY look for the atom in db or by issuing a service bus command accordingly.
For LOCAL_AND_REMOTE, look for the atom/link/node in the db. If present, return it. Otherwise, issue a
command in the bus.

```
* pattern_matching_query(query, answers_count = 0)
* pattern_matching_count(query)
* pattern_matching_fetch(query, answers_count = 0)
```

Queries are issued in the service bus

```
* char*  add_node(type, name)
* char*  add_link(type, *targets list*)
```

Atoms are instantiated inside add_node() and add_link(). Atom handle is computed (and NOT stored in the
Atom object). Atom pointer is stored directly in the HandleTrie using the computed handle as key.

```
* void commit_changes()
```

If LOCAL scope is selected (LOCAL_ONLY or LOCAL_AND_REMOTE), commit ALL atoms in local db by sending them
to the server (by issuing a command in the service bus). Atoms should be kept in the local db. The
command should wait this sync ends before going to the next step.

Once local changes are committed, issue a DB COMMIT command in the service bus.

*/

#pragma once

#include <memory>

#include "AtomDB.h"
#include "HandleTrie.h"
#include "PatternMatchingQueryProxy.h"
#include "ServiceBus.h"

#define HANDLE_SIZE 32

using namespace std;
using namespace atomdb;
using namespace commons;
using namespace service_bus;
using namespace query_engine;

namespace atomspace {

class AtomSpace {
   public:
    enum Scope { LOCAL_ONLY, REMOTE_ONLY, LOCAL_AND_REMOTE };

    AtomSpace(shared_ptr<AtomDB> db, shared_ptr<ServiceBus> bus)
        : db_(move(db)), bus_(move(bus)), handle_trie_(make_unique<HandleTrie>(HANDLE_SIZE)) {}

    const Atom& get_atom(const char* handle, Scope scope = LOCAL_AND_REMOTE);
    const Link& get_link(const string& type,
                         const vector<string>& targets,
                         Scope scope = LOCAL_AND_REMOTE);
    const Node& get_node(const string& type, const string& name, Scope scope = LOCAL_AND_REMOTE);

    shared_ptr<PatternMatchingQueryProxy> pattern_matching_query(const vector<string>& query,
                                                                 unsigned int answers_count = 0);
    unsigned int pattern_matching_count(const vector<string>& query);
    void pattern_matching_fetch(const vector<string>& query, unsigned int answers_count = 0);

    char* add_node(const string& type, const string& name);
    char* add_link(const string& type, const vector<string>& targets);

    void commit_changes(Scope scope = LOCAL_AND_REMOTE);

   private:
    shared_ptr<AtomDB> db_;
    shared_ptr<ServiceBus> bus_;
    unique_ptr<HandleTrie> handle_trie_;
};

}  // namespace atomspace