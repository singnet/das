#pragma once

#include "Atom.h"

namespace commons {

/**
 * Abstract interface supposed to be implemented by classes that are capable of
 * making an Atom out of a Handle.
 */
class HandleDecoder {
   protected:
    HandleDecoder() {}

   public:
    ~HandleDecoder() {}
    virtual shared_ptr<Atom> get_atom(const string& handle) = 0;
};

}  // namespace commons
