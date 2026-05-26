#pragma once

#include <algorithm>
#include <string>
#include <tuple>
#include <unordered_set>
#include <variant>
#include <vector>

#include "DatabaseTypes.h"
#include "HandleTrie.h"
#include "MettaMapping.h"
#include "MettaParserActions.h"

using namespace std;
using namespace atoms;
using namespace commons;

namespace db_adapter {

/**
 * @class DatabaseMapper
 * @brief Abstract base class for transforming database input into a target representation.
 */
class DatabaseMapper {
   public:
    virtual ~DatabaseMapper() = default;

    /**
     * @brief Transforms the input data into a list of Atom pointers.
     * @param data The database row or document to map.
     * @return vector<shared_ptr<Atom>> A list of Atom pointers.
     */
    virtual vector<shared_ptr<Atom>> map(const DbInput& data) = 0;

    unsigned int handle_trie_size();

   protected:
    DatabaseMapper() = default;
    HandleTrie handle_trie{32};
};

}  // namespace db_adapter