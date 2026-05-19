#include "DatabaseMapper.h"

#include <iostream>
#include <variant>

#include "Atom.h"
#include "DatabaseTypes.h"
#include "Hasher.h"
#include "Link.h"
#include "MettaMapping.h"
#include "MettaParser.h"
#include "Node.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace db_adapter;
using namespace commons;
using namespace atoms;

unsigned int DatabaseMapper::handle_trie_size() { return this->handle_trie.size; }
