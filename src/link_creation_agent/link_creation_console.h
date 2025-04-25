/**
 * @file link_creation_console.h
 */

#pragma once

#include <memory>
#include <string>

#include "AtomDB.h"
#include "AtomDBAPITypes.h"
#include "AtomDBSingleton.h"
#include "link.h"
#include "variant"

using namespace atomdb;
using namespace std;

namespace link_creation_agent {

class Console {
   public:
    static shared_ptr<Console> get_instance();
    ~Console() {}
    string print_metta(std::vector<string> tokens);
    LinkTargetTypes get_atom(std::string handle);

   private:
    Console(){};
    static shared_ptr<Console> console_instance;
};

}  // namespace link_creation_agent