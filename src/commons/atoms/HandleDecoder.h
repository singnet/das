#pragma once

#include "Atom.h"
#include "Utils.h"

namespace atoms {

/**
 * Abstract interface supposed to be implemented by classes that are capable of
 * making an Atom out of a Handle.
 */
class HandleDecoder {
   protected:
    HandleDecoder() {}

   public:
    virtual ~HandleDecoder() {}
    virtual shared_ptr<Atom> get_atom(const string& handle) = 0;
};

}  // namespace atoms
