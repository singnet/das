#pragma once

#include <AtomDB.h>

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "HandleTrie.h"

using namespace std;
using namespace atomdb;

namespace db_adapter {

class AtomProcessor {
   public:
    AtomProcessor();
    ~AtomProcessor();

    void process_atoms(vector<Atom*> atoms);
    bool has_handle(const string& handle);

   private:
    vector<Atom*> atoms;
    unordered_set<string> handle_set;
    shared_ptr<AtomDB> atomdb;
};

}  // namespace db_adapter