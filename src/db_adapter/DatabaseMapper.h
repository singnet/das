#pragma once

#include <algorithm>
#include <memory>
#include <queue>
#include <string>
#include <tuple>
#include <unordered_set>
#include <variant>
#include <vector>

#include "Atom.h"
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
     * @brief Maps the database input to a queue of Atom pointers.
     * @param data The database row, document to map or other input.
     * @param output The queue to store the resulting Atom pointers.
     */
    virtual void map(const DbInput& data, std::queue<shared_ptr<Atom>>& output) = 0;

    unsigned int handle_trie_size();

   protected:
    DatabaseMapper() = default;
    HandleTrie handle_trie{32};
};

}  // namespace db_adapter