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

   protected:
    Context(const string& name);

   private:
    void set_determiners();
    string name;
    string key;
};

}  // namespace atom_space
