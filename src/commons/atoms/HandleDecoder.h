#pragma once

#include "Atom.h"
#include "Utils.h"
#include "Keychain.h"

namespace atoms {

/**
 * Abstract interface supposed to be implemented by classes that are capable of
 * making an Atom (or getting atom's basic info) out of a Handle.
 */
class HandleDecoder {
   protected:
    HandleDecoder() {}

   public:
    virtual ~HandleDecoder() {}
    virtual shared_ptr<Atom> get_atom(const string& handle, shared_ptr<Keychain> keychain = nullptr) = 0;
    virtual string get_name(const string& handle, shared_ptr<Keychain> keychain = nullptr) = 0;
    virtual vector<string> get_targets(const string& handle, shared_ptr<Keychain> keychain = nullptr) = 0;
};

}  // namespace atoms
