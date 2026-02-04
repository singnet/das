#pragma once

#include <algorithm>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

#include "DataTypes.h"
#include "HandleTrie.h"
#include "MettaMapping.h"

using namespace std;
using namespace atoms;
using namespace commons;

namespace db_adapter {

class MapperValue : public HandleTrie::TrieValue {
   public:
    MapperValue() {}
    void merge(TrieValue* other) {}
};

/**
 * @class Mapper
 * @brief Abstract base class for transforming database input into a target representation.
 */
class Mapper {
   public:
    virtual ~Mapper() = default;

    /**
     * @brief Transforms the input data into the output format.
     * @param data The database row or document to map.
     * @return OutputList A variant containing either strings or Atoms.
     */
    virtual const OutputList map(const DbInput& data) = 0;

    unsigned int handle_trie_size() { return handle_trie.size; }

   protected:
    Mapper() = default;
    HandleTrie handle_trie{32};
};

/**
 * @class BaseSQL2Mapper
 * @brief Common logic for mapping SQL data, handling Primary and Foreign keys.
 */
class BaseSQL2Mapper : public Mapper {
   public:
    virtual ~BaseSQL2Mapper() override = default;

    const OutputList map(const DbInput& data) override;

    static string SYMBOL;
    static string EXPRESSION;

    static void initialize_statics() {
        SYMBOL = MettaMapping::SYMBOL_NODE_TYPE;
        EXPRESSION = MettaMapping::EXPRESSION_LINK_TYPE;
    }

   protected:
    BaseSQL2Mapper() = default;

    bool is_foreign_key(const string& column_name);
    string escape_inner_quotes(string text);
    virtual OutputList get_output() = 0;
    virtual void clear() = 0;
    bool insert_handle_if_missing(const string& handle);

    virtual void map_primary_key(const string& table_name, const string& primary_key_value) = 0;
    virtual void map_foreign_key_column(const string& table_name,
                                        const string& column_name,
                                        const string& value,
                                        const string& primary_key_value,
                                        const string& fk_table,
                                        const string& fk_column) = 0;
    virtual void map_regular_column(const string& table_name,
                                    const string& column_name,
                                    const string& value,
                                    const string& primary_key_value) = 0;
    virtual void map_foreign_keys_combinations(
        const vector<tuple<string, string, string>>& all_foreign_keys) = 0;
};

/**
 * @class SQL2MettaMapper
 * @brief Maps SQL rows to Metta S-Expressions.
 */
class SQL2MettaMapper : public BaseSQL2Mapper {
   public:
    SQL2MettaMapper();
    ~SQL2MettaMapper() override;

   private:
    vector<string> metta_expressions;
    OutputList get_output() override;
    void clear() override;
    void add_metta_if_new(const string& s_expression);

    void map_primary_key(const string& table_name, const string& primary_key_value) override;
    void map_foreign_key_column(const string& table_name,
                                const string& column_name,
                                const string& value,
                                const string& primary_key_value,
                                const string& fk_table,
                                const string& fk_column) override;
    void map_regular_column(const string& table_name,
                            const string& column_name,
                            const string& value,
                            const string& primary_key_value) override;
    void map_foreign_keys_combinations(
        const vector<tuple<string, string, string>>& all_foreign_keys) override;
};

/**
 * @class SQL2AtomsMapper
 * @brief Maps SQL rows to Atom objects.
 */
class SQL2AtomsMapper : public BaseSQL2Mapper {
   public:
    SQL2AtomsMapper();
    ~SQL2AtomsMapper() override;

    enum ATOM_TYPE { NODE, LINK };

   private:
    vector<Atom*> atoms;

    OutputList get_output() override;
    void clear() override;
    string add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE atom_type,
                           variant<string, vector<string>> value,
                           bool is_toplevel = false);

    void map_primary_key(const string& table_name, const string& primary_key_value) override;
    void map_foreign_key_column(const string& table_name,
                                const string& column_name,
                                const string& value,
                                const string& primary_key_value,
                                const string& fk_table,
                                const string& fk_column) override;
    void map_regular_column(const string& table_name,
                            const string& column_name,
                            const string& value,
                            const string& primary_key_value) override;
    void map_foreign_keys_combinations(
        const vector<tuple<string, string, string>>& all_foreign_keys) override;
};

}  // namespace db_adapter