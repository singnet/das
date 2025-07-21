/**
 * @file LinkCreationDBHelper.h
 */

#pragma once

#include <string>

#include "AtomDB.h"
#include "AtomDBAPITypes.h"
#include "AtomDBSingleton.h"
#include "LCALink.h"

using namespace atomdb;
using namespace std;

namespace link_creation_agent {

class LinkCreationDBWrapper {
   public:
    LinkCreationDBWrapper() = default;
    ~LinkCreationDBWrapper() = default;
    static void print_metta(vector<string> tokens, bool has_custom_field_size = true);
    static string tokens_to_metta_string(vector<string> tokens, bool has_custom_field_size = true);
    static LinkTargetTypes get_atom(string handle);
};

}  // namespace link_creation_agent