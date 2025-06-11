/**
 * @file link_creation_db_helper.h
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

class LinkCreateDBSingleton {
   public:
    static shared_ptr<LinkCreateDBSingleton> get_instance();
    ~LinkCreateDBSingleton() {}
    void print_metta(vector<string> tokens, bool has_custom_field_size = true);
    string tokens_to_metta_string(vector<string> tokens, bool has_custom_field_size = true);
    LinkTargetTypes get_atom(string handle);

   private:
    LinkCreateDBSingleton(){};
    static shared_ptr<LinkCreateDBSingleton> instance;
};

}  // namespace link_creation_agent