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

using namespace std;
using namespace atoms;
using namespace commons;

namespace db_adapter {

/**
 * @class Mapper
 * @brief Abstract base class for transforming database input into a target representation.
 */
class Mapper {
   public:
    virtual ~Mapper() = default;

    /**
     * @brief Transforms the input data into a list of Atom pointers.
     * @param data The database row or document to map.
     * @return vector<Atom*> A list of Atom pointers.
     */
    virtual const vector<Atom*> map(const DbInput& data) = 0;

    unsigned int handle_trie_size() { return handle_trie.size; }

   protected:
    Mapper() = default;
    HandleTrie handle_trie{32};
};

/**
 * @class SQL2AtomsMapper
 * @brief Maps SQL rows to Atom objects.
 */
class SQL2AtomsMapper : public Mapper {
   public:
    SQL2AtomsMapper();
    ~SQL2AtomsMapper() override;

    enum ATOM_TYPE { NODE, LINK };

    static string SYMBOL;
    static string EXPRESSION;

    static void initialize_statics() {
        SYMBOL = MettaMapping::SYMBOL_NODE_TYPE;
        EXPRESSION = MettaMapping::EXPRESSION_LINK_TYPE;
    }

    const vector<Atom*> map(const DbInput& data) override;

   private:
    vector<Atom*> atoms;

    bool is_foreign_key(const string& column_name);
    string escape_inner_quotes(string text);
    bool insert_handle_if_missing(const string& handle);
    string add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE atom_type,
                           variant<string, vector<string>> value,
                           const string& metta_expression = "",
                           bool is_toplevel = false);

    void map_primary_key(const string& table_name, const string& primary_key_value);
    void map_foreign_key_column(const string& table_name,
                                const string& column_name,
                                const string& value,
                                const string& primary_key_value,
                                const string& fk_table,
                                const string& fk_column);
    void map_regular_column(const string& table_name,
                            const string& column_name,
                            const string& value,
                            const string& primary_key_value);
    void map_foreign_keys_combinations(const vector<tuple<string, string, string>>& all_foreign_keys);
};

}  // namespace db_adapter