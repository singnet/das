#include "Sql2AtomsMapper.h"

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

string SQL2AtomsMapper::SYMBOL;
string SQL2AtomsMapper::EXPRESSION;

// ==============================
//  Construction / destruction
// ==============================

SQL2AtomsMapper::SQL2AtomsMapper() { this->initialize_statics(); }

SQL2AtomsMapper::~SQL2AtomsMapper() {}

// ==============================
//  Public
// ==============================

void SQL2AtomsMapper::map(const DbInput& data, std::queue<shared_ptr<Atom>>& output) {
    vector<tuple<string, string, string>> all_foreign_keys;
    SqlRow sql_row = get<SqlRow>(data);
    string table_name = sql_row.table_name;

    string primary_key_value = sql_row.primary_key ? sql_row.primary_key->value : "";

    this->map_primary_key(table_name, primary_key_value, output);

    for (auto& field : sql_row.fields) {
        string column_name = table_name + "." + field.name;

        if (this->is_foreign_key(column_name)) {
            size_t separator_pos = column_name.find('|');

            if (separator_pos == string::npos) {
                RAISE_ERROR("Invalid foreign key format: " + column_name);
            }

            string fk_column = column_name.substr(0, separator_pos);
            string fk_table = column_name.substr(separator_pos + 1);
            all_foreign_keys.push_back({fk_column, fk_table, field.value});

            this->map_foreign_key_column(
                table_name, column_name, field.value, primary_key_value, fk_table, fk_column, output);
        } else {
            this->map_regular_column(table_name, column_name, field.value, primary_key_value, output);
        };
    }

    this->map_foreign_keys_combinations(all_foreign_keys, output);
}

// ==============================
// Private
// ==============================

bool SQL2AtomsMapper::is_foreign_key(const string& column_name) {
    size_t pos = column_name.find('|');
    if (pos == string::npos) return false;
    return true;
}

string SQL2AtomsMapper::escape_inner_quotes(string text) {
    const auto count = std::count(text.begin(), text.end(), '"');

    if (count == 0 || count == 2) return text;

    if (count == 1) {
        text.insert(text.begin(), '"');
        text.push_back('"');
        return text;
    }

    if (text.size() >= 2 && text.front() == '"' && text.back() == '"') {
        string inner = text.substr(1, text.size() - 2);

        string escaped;
        escaped.reserve(inner.size());
        for (char c : inner) {
            if (c == '"') {
                escaped.push_back('\\');
            }
            escaped.push_back(c);
        }

        return "\"" + escaped + "\"";
    }
    return "";
}

bool SQL2AtomsMapper::insert_handle_if_missing(const string& handle) {
    auto exists = this->handle_trie.exists(handle);
    if (exists) return false;
    this->handle_trie.insert(handle, new EmptyTrieValue());
    return true;
}

string SQL2AtomsMapper::add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE atom_type,
                                        variant<string, vector<string>> value,
                                        std::queue<shared_ptr<Atom>>& output,
                                        const string& metta_expression,
                                        bool is_toplevel) {
    shared_ptr<Atom> atom;

    if (atom_type == NODE) {
        string name = get<string>(value);
        atom = make_shared<Node>(SQL2AtomsMapper::SYMBOL, name);
    } else if (atom_type == LINK) {
        vector<string> targets = get<vector<string>>(value);
        atom = make_shared<Link>(SQL2AtomsMapper::EXPRESSION, targets, is_toplevel);
    } else {
        RAISE_ERROR("Either name or targets must be provided to create an Atom.");
    }

    if (!metta_expression.empty()) {
        atom->custom_attributes["metta_expression"] = metta_expression;
    }

    output.push(atom);
    return atom->handle();
};

void SQL2AtomsMapper::map_primary_key(const string& table_name,
                                      const string& primary_key_value,
                                      std::queue<shared_ptr<Atom>>& output) {
    string literal_pk = this->escape_inner_quotes("\"" + primary_key_value + "\"");

    // Nodes
    string predicate_node_h = this->add_atom_if_new(NODE, string("Predicate"), output);
    string concept_node_h = this->add_atom_if_new(NODE, string("Concept"), output);
    string evaluation_node_h = this->add_atom_if_new(NODE, string("Evaluation"), output);
    string literal_pk_node_h = this->add_atom_if_new(NODE, string(literal_pk), output);
    string table_name_node_h = this->add_atom_if_new(NODE, string(table_name), output);

    // Links
    string predicate_link_h = this->add_atom_if_new(LINK,
                                                    vector<string>{predicate_node_h, table_name_node_h},
                                                    output,
                                                    "(Predicate " + table_name + ")");
    string concept_inner_link_h =
        this->add_atom_if_new(LINK,
                              vector<string>{table_name_node_h, literal_pk_node_h},
                              output,
                              "(" + table_name + " " + literal_pk + ")");
    string concept_link_h = this->add_atom_if_new(LINK,
                                                  vector<string>{concept_node_h, concept_inner_link_h},
                                                  output,
                                                  "(Concept (" + table_name + " " + literal_pk + "))");
    this->add_atom_if_new(
        LINK,
        vector<string>{evaluation_node_h, predicate_link_h, concept_link_h},
        output,
        "(Evaluation (Predicate " + table_name + ") (Concept (" + table_name + " " + literal_pk + ")))",
        true);
}

void SQL2AtomsMapper::map_foreign_key_column(const string& table_name,
                                             const string& column_name,
                                             const string& value,
                                             const string& primary_key_value,
                                             const string& fk_table,
                                             const string& fk_column,
                                             std::queue<shared_ptr<Atom>>& output) {
    string literal_value = this->escape_inner_quotes("\"" + value + "\"");
    string literal_pk = this->escape_inner_quotes("\"" + primary_key_value + "\"");

    // Nodes
    string predicate_node_h = this->add_atom_if_new(NODE, string("Predicate"), output);
    string concept_node_h = this->add_atom_if_new(NODE, string("Concept"), output);
    string evaluation_node_h = this->add_atom_if_new(NODE, string("Evaluation"), output);
    string literal_pk_node_h = this->add_atom_if_new(NODE, string(literal_pk), output);
    string table_name_node_h = this->add_atom_if_new(NODE, string(table_name), output);
    string fk_table_node_h = this->add_atom_if_new(NODE, string(fk_table), output);
    string literal_value_node_h = this->add_atom_if_new(NODE, string(literal_value), output);
    string fk_column_node_h = this->add_atom_if_new(NODE, string(fk_column), output);

    // Links
    string predicate_inner_1_link_h =
        this->add_atom_if_new(LINK,
                              vector<string>{fk_table_node_h, literal_value_node_h},
                              output,
                              "(" + fk_table + " " + literal_value + ")");
    string predicate_inner_2_link_h =
        this->add_atom_if_new(LINK,
                              vector<string>{concept_node_h, predicate_inner_1_link_h},
                              output,
                              "(Concept (" + fk_table + " " + literal_value + "))");
    string predicate_inner_3_link_h =
        this->add_atom_if_new(LINK,
                              vector<string>{fk_column_node_h, predicate_inner_2_link_h},
                              output,
                              "(" + fk_column + " (Concept (" + fk_table + " " + literal_value + ")))");
    string predicate_link_h = this->add_atom_if_new(
        LINK,
        vector<string>{predicate_node_h, predicate_inner_3_link_h},
        output,
        "(Predicate (" + fk_column + " (Concept (" + fk_table + " " + literal_value + "))))");
    string concept_inner_link_h =
        this->add_atom_if_new(LINK,
                              vector<string>{table_name_node_h, literal_pk_node_h},
                              output,
                              "(" + table_name + " " + literal_pk + ")");
    string concept_link_h = this->add_atom_if_new(LINK,
                                                  vector<string>{concept_node_h, concept_inner_link_h},
                                                  output,
                                                  "(Concept (" + table_name + " " + literal_pk + "))");

    this->add_atom_if_new(LINK,
                          vector<string>{evaluation_node_h, predicate_link_h, concept_link_h},
                          output,
                          "(Evaluation (Predicate (" + fk_column + " (Concept (" + fk_table + " " +
                              literal_value + ")))) (Concept (" + table_name + " " + literal_pk + ")))",
                          true);
}

void SQL2AtomsMapper::map_regular_column(const string& table_name,
                                         const string& column_name,
                                         const string& value,
                                         const string& primary_key_value,
                                         std::queue<shared_ptr<Atom>>& output) {
    string literal_pk = this->escape_inner_quotes("\"" + primary_key_value + "\"");
    string literal_value = this->escape_inner_quotes("\"" + value + "\"");

    // Nodes
    string predicate_node_h = this->add_atom_if_new(NODE, string("Predicate"), output);
    string concept_node_h = this->add_atom_if_new(NODE, string("Concept"), output);
    string evaluation_node_h = this->add_atom_if_new(NODE, string("Evaluation"), output);
    string table_name_node_h = this->add_atom_if_new(NODE, string(table_name), output);
    string column_name_node_h = this->add_atom_if_new(NODE, string(column_name), output);
    string literal_value_node_h = this->add_atom_if_new(NODE, string(literal_value), output);
    string literal_pk_node_h = this->add_atom_if_new(NODE, string(literal_pk), output);

    // Links
    string predicate_inner_link_h = this->add_atom_if_new(
        LINK,
        vector<string>{table_name_node_h, column_name_node_h, literal_value_node_h},
        output,
        "(" + table_name + " " + column_name + " " + literal_value + ")");
    string predicate_link_h = this->add_atom_if_new(
        LINK,
        vector<string>{predicate_node_h, predicate_inner_link_h},
        output,
        "(Predicate (" + table_name + " " + column_name + " " + literal_value + "))");
    string concept_inner_link_h =
        this->add_atom_if_new(LINK,
                              vector<string>{table_name_node_h, literal_pk_node_h},
                              output,
                              "(" + table_name + " " + literal_pk + ")");
    string concept_link_h = this->add_atom_if_new(LINK,
                                                  vector<string>{concept_node_h, concept_inner_link_h},
                                                  output,
                                                  "(Concept (" + table_name + " " + literal_pk + "))");
    this->add_atom_if_new(LINK,
                          vector<string>{evaluation_node_h, predicate_link_h, concept_link_h},
                          output,
                          "(Evaluation (Predicate (" + table_name + " " + column_name + " " +
                              literal_value + ")) (Concept (" + table_name + " " + literal_pk + ")))",
                          true);
}

void SQL2AtomsMapper::map_foreign_keys_combinations(
    const vector<tuple<string, string, string>>& all_foreign_keys,
    std::queue<shared_ptr<Atom>>& output) {
    for (const auto& [column_name, foreign_table_name, value] : all_foreign_keys) {
        for (const auto& [column_name2, foreign_table_name2, value2] : all_foreign_keys) {
            if (make_pair(foreign_table_name, value) != make_pair(foreign_table_name2, value2)) {
                string literal_value = this->escape_inner_quotes("\"" + value + "\"");
                string literal_value_2 = this->escape_inner_quotes("\"" + value2 + "\"");

                // Nodes
                string predicate_node_h = this->add_atom_if_new(NODE, string("Predicate"), output);
                string concept_node_h = this->add_atom_if_new(NODE, string("Concept"), output);
                string evaluation_node_h = this->add_atom_if_new(NODE, string("Evaluation"), output);
                string fk_column_node_h = this->add_atom_if_new(NODE, string(column_name), output);
                string foreign_table_name_node_h =
                    this->add_atom_if_new(NODE, string(foreign_table_name), output);
                string literal_value_node_h = this->add_atom_if_new(NODE, string(literal_value), output);
                string foreign_table_name2_node_h =
                    this->add_atom_if_new(NODE, string(foreign_table_name2), output);
                string literal_value2_node_h =
                    this->add_atom_if_new(NODE, string(literal_value_2), output);
                // Links
                string predicate_inner_1_link_h = this->add_atom_if_new(
                    LINK,
                    vector<string>{foreign_table_name_node_h, literal_value_node_h},
                    output,
                    "(" + foreign_table_name + " " + literal_value + ")");
                string predicate_inner_2_link_h = this->add_atom_if_new(
                    LINK,
                    vector<string>{concept_node_h, predicate_inner_1_link_h},
                    output,
                    "(Concept (" + foreign_table_name + " " + literal_value + "))");
                string predicate_inner_3_link_h =
                    this->add_atom_if_new(LINK,
                                          vector<string>{fk_column_node_h, predicate_inner_2_link_h},
                                          output,
                                          "(" + column_name + " (Concept (" + foreign_table_name + " " +
                                              literal_value + ")))");
                string predicate_link_h =
                    this->add_atom_if_new(LINK,
                                          vector<string>{predicate_node_h, predicate_inner_3_link_h},
                                          output,
                                          "(Predicate (" + column_name + " (Concept (" +
                                              foreign_table_name + " " + literal_value + "))))");
                string concept_inner_link_h = this->add_atom_if_new(
                    LINK,
                    vector<string>{foreign_table_name2_node_h, literal_value2_node_h},
                    output,
                    "(" + foreign_table_name2 + " " + literal_value_2 + ")");
                string concept_link_h = this->add_atom_if_new(
                    LINK,
                    vector<string>{concept_node_h, concept_inner_link_h},
                    output,
                    "(Concept (" + foreign_table_name2 + " " + literal_value_2 + "))");

                this->add_atom_if_new(
                    LINK,
                    vector<string>{evaluation_node_h, predicate_link_h, concept_link_h},
                    output,
                    "(Evaluation (Predicate (" + column_name + " (Concept (" + foreign_table_name + " " +
                        literal_value + ")))) (Concept (" + foreign_table_name2 + " " + literal_value_2 +
                        ")))",
                    true);
            }
        }
    }
}
