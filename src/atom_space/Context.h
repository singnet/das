#pragma once

#include <string>

using namespace std;

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

   private:
    Context(const string& name);
    string name;
    string key;
};

}  // namespace atom_space
