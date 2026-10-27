#pragma once

#include <string>

#include "AtomDB.h"

namespace command_router {

/** Insert the atoms of a MeTTa expression. Returns the top-level atom handle. */
std::string add_metta_expression(atomdb::AtomDB& db, const std::string& expression);

/** Handle of the top-level atom a MeTTa expression names. Does not write to AtomDB. */
std::string metta_expression_root_handle(const std::string& expression);

}  // namespace command_router
