#pragma once

#include <string>

#include "Atom.h"

using namespace std;
using namespace atoms;

namespace atom_space {

/**
 *
 */
class Context {
    friend class AtomSpace;

   public:
    ~Context();
    const string& get_key();
    const string& get_name();

   protected:
    Context(const string& name, Atom& atom_key);

   private:
    void set_determiners(Atom& atom_key);
    string name;
    string key;
};

}  // namespace atom_space
