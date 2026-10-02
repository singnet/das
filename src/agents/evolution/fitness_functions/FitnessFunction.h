#pragma once

#include <memory>

#include "AtomDB.h"
#include "AtomDBSingleton.h"
#include "KeySensitiveAtomDB.h"
#include "Keychain.h"
#include "QueryAnswer.h"

using namespace std;
using namespace query_engine;
using namespace atomdb;

namespace fitness_functions {

/**
 * Abstract superclass for fitness functions used to evolve queries.
 */
class FitnessFunction {
   public:
    FitnessFunction() {
        this->atomdb = AtomDBSingleton::get_instance();
        this->key_sensitive_atomdb = dynamic_pointer_cast<KeySensitiveAtomDB>(atomdb);
    }
    virtual ~FitnessFunction() {}

    virtual float eval(shared_ptr<QueryAnswer> query_answer,
                       shared_ptr<Keychain> keychain = nullptr) = 0;

   protected:
    shared_ptr<AtomDB> atomdb;
    shared_ptr<KeySensitiveAtomDB> key_sensitive_atomdb;
};

}  // namespace fitness_functions
