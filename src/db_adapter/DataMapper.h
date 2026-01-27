#pragma once

#include <algorithm>
#include <string>

#include "DataTypes.h"
#include "HandleTrie.h"

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
    virtual const db_adapter_types::OutputList map(db_adapter_types::DbInput& data) = 0;

   protected:
    Mapper() = default;
    HandleTrie handle_trie{32};
    MapperValue* default_mapper_value;
};

class BaseSQL2Mapper : public Mapper {
   public:
    virtual ~BaseSQL2Mapper() override = default;
    const db_adapter_types::OutputList map(db_adapter_types::DbInput& data) override;

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
    virtual db_adapter_types::OutputList get_output() = 0;
    bool insert_handle_if_missing(const string& handle);

    virtual void process_primary_key(string& table_name, string& primary_key_value) = 0;
    virtual void process_foreign_key_column(string& table_name,
                                            string& column_name,
                                            string& value,
                                            string& primary_key_value,
                                            vector<tuple<string, string, string>>& all_foreign_keys) = 0;
    virtual void process_regular_column(string& table_name,
                                        string& column_name,
                                        string& value,
                                        string& primary_key_value) = 0;
    virtual void process_foreign_keys_combinations(
        vector<tuple<string, string, string>> all_foreign_keys) = 0;
};

class SQL2MettaMapper : public BaseSQL2Mapper {
   public:
    SQL2MettaMapper();
    ~SQL2MettaMapper() override;

   private:
    vector<string> metta_expressions;
    db_adapter_types::OutputList get_output() override;
    void add_metta_if_new(const string& s_expression);

    void process_primary_key(string& table_name, string& primary_key_value) override;
    void process_foreign_key_column(string& table_name,
                                    string& column_name,
                                    string& value,
                                    string& primary_key_value,
                                    vector<tuple<string, string, string>>& all_foreign_keys) override;
    void process_regular_column(string& table_name,
                                string& column_name,
                                string& value,
                                string& primary_key_value) override;
    void process_foreign_keys_combinations(
        vector<tuple<string, string, string>> all_foreign_keys) override;
};

class SQL2AtomsMapper : public BaseSQL2Mapper {
   public:
    SQL2AtomsMapper();
    ~SQL2AtomsMapper() override;

   private:
    vector<Atom*> atoms;
    db_adapter_types::OutputList get_output() override;
    string add_atom_if_new(const string& name = "",
                           const vector<string>& targets = {},
                           bool is_toplevel = false);

    void process_primary_key(string& table_name, string& primary_key_value) override;
    void process_foreign_key_column(string& table_name,
                                    string& column_name,
                                    string& value,
                                    string& primary_key_value,
                                    vector<tuple<string, string, string>>& all_foreign_keys) override;
    void process_regular_column(string& table_name,
                                string& column_name,
                                string& value,
                                string& primary_key_value) override;
    void process_foreign_keys_combinations(
        vector<tuple<string, string, string>> all_foreign_keys) override;
};

}  // namespace db_adapter
