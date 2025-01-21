#pragma once

#include <database.h>

using namespace atomdb;

/**
 * @class AtomDBPublicist
 * @brief A publicist class for AtomDB that exposes certain protected member functions.
 *
 * This class inherits from AtomDB and makes the following protected member functions
 * accessible publicly:
 * - _build_link
 * - _build_node
 * - _get_atom
 *
 * This can be useful for testing or other purposes where access to these functions is required.
 */
class AtomDBPublicist : public AtomDB {
   public:
    using AtomDB::_build_link;
    using AtomDB::_build_node;
    using AtomDB::_get_atom;
};
