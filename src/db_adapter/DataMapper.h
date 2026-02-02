#pragma once

#include <algorithm>
#include <string>

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

class Mapper {
   public:
    virtual ~Mapper() = default;
    virtual const OutputList map(const DbInput& data) = 0;

   protected:
    Mapper() = default;
    HandleTrie handle_trie{32};
};

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

    bool is_foreign_key(string& column_name);
    string escape_inner_quotes(string text);
    virtual OutputList get_output() = 0;
    bool insert_handle_if_missing(const string& handle);

    virtual void map_primary_key(string& table_name, string& primary_key_value) = 0;
    virtual void map_foreign_key_column(string& table_name,
                                        string& column_name,
                                        string& value,
                                        string& primary_key_value,
                                        string& fk_table,
                                        string& fk_column) = 0;
    virtual void map_regular_column(string& table_name,
                                    string& column_name,
                                    string& value,
                                    string& primary_key_value) = 0;
    virtual void map_foreign_keys_combinations(
        vector<tuple<string, string, string>> all_foreign_keys) = 0;
};

class SQL2MettaMapper : public BaseSQL2Mapper {
   public:
    SQL2MettaMapper();
    ~SQL2MettaMapper() override;

   private:
    vector<string> metta_expressions;
    OutputList get_output() override;
    void add_metta_if_new(const string& s_expression);

    void map_primary_key(string& table_name, string& primary_key_value) override;
    void map_foreign_key_column(string& table_name,
                                string& column_name,
                                string& value,
                                string& primary_key_value,
                                string& fk_table,
                                string& fk_column) override;
    void map_regular_column(string& table_name,
                            string& column_name,
                            string& value,
                            string& primary_key_value) override;
    void map_foreign_keys_combinations(vector<tuple<string, string, string>> all_foreign_keys) override;
};

class SQL2AtomsMapper : public BaseSQL2Mapper {
   public:
    SQL2AtomsMapper();
    ~SQL2AtomsMapper() override;

    enum ATOM_TYPE { NODE, LINK };

   private:
    vector<Atom*> atoms;
    OutputList get_output() override;
    string add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE atom_type,
                           variant<string, vector<string>> value,
                           bool is_toplevel = false);

    void map_primary_key(string& table_name, string& primary_key_value) override;
    void map_foreign_key_column(string& table_name,
                                string& column_name,
                                string& value,
                                string& primary_key_value,
                                string& fk_table,
                                string& fk_column) override;
    void map_regular_column(string& table_name,
                            string& column_name,
                            string& value,
                            string& primary_key_value) override;
    void map_foreign_keys_combinations(vector<tuple<string, string, string>> all_foreign_keys) override;
};

}  // namespace db_adapter
