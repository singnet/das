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

const vector<Atom*> SQL2AtomsMapper::map(const DbInput& data) {
    vector<tuple<string, string, string>> all_foreign_keys;
    SqlRow sql_row = get<SqlRow>(data);
    string table_name = sql_row.table_name;

    string primary_key_value = sql_row.primary_key ? sql_row.primary_key->value : "";

    this->map_primary_key(table_name, primary_key_value);

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
                table_name, column_name, field.value, primary_key_value, fk_table, fk_column);
        } else {
            this->map_regular_column(table_name, column_name, field.value, primary_key_value);
        };
    }

    this->map_foreign_keys_combinations(all_foreign_keys);

    auto result = move(this->atoms);
    this->atoms.clear();
    return result;
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
                                        const string& metta_expression,
                                        bool is_toplevel) {
    Atom* atom;

    if (atom_type == NODE) {
        string name = get<string>(value);
        atom = new Node(SQL2AtomsMapper::SYMBOL, name);
    } else if (atom_type == LINK) {
        vector<string> targets = get<vector<string>>(value);
        atom = new Link(SQL2AtomsMapper::EXPRESSION, targets, is_toplevel);
    } else {
        RAISE_ERROR("Either name or targets must be provided to create an Atom.");
    }

    if (!metta_expression.empty()) {
        atom->custom_attributes["metta_expression"] = metta_expression;
    }

    this->atoms.push_back(atom);
    return atom->handle();
};

void SQL2AtomsMapper::map_primary_key(const string& table_name, const string& primary_key_value) {
    string literal_pk = this->escape_inner_quotes("\"" + primary_key_value + "\"");

    // Nodes
    string predicate_node_h = this->add_atom_if_new(NODE, string("Predicate"));
    string concept_node_h = this->add_atom_if_new(NODE, string("Concept"));
    string evaluation_node_h = this->add_atom_if_new(NODE, string("Evaluation"));
    string literal_pk_node_h = this->add_atom_if_new(NODE, string(literal_pk));
    string table_name_node_h = this->add_atom_if_new(NODE, string(table_name));

    // Links
    string predicate_link_h = this->add_atom_if_new(
        LINK, vector<string>{predicate_node_h, table_name_node_h}, "(Predicate " + table_name + ")");
    string concept_inner_link_h =
        this->add_atom_if_new(LINK,
                              vector<string>{table_name_node_h, literal_pk_node_h},
                              "(" + table_name + " " + literal_pk + ")");
    string concept_link_h = this->add_atom_if_new(LINK,
                                                  vector<string>{concept_node_h, concept_inner_link_h},
                                                  "(Concept (" + table_name + " " + literal_pk + "))");
    this->add_atom_if_new(
        LINK,
        vector<string>{evaluation_node_h, predicate_link_h, concept_link_h},
        "(Evaluation (Predicate " + table_name + ") (Concept (" + table_name + " " + literal_pk + ")))",
        true);
}

void SQL2AtomsMapper::map_foreign_key_column(const string& table_name,
                                             const string& column_name,
                                             const string& value,
                                             const string& primary_key_value,
                                             const string& fk_table,
                                             const string& fk_column) {
    string literal_value = this->escape_inner_quotes("\"" + value + "\"");
    string literal_pk = this->escape_inner_quotes("\"" + primary_key_value + "\"");

    // Nodes
    string predicate_node_h = this->add_atom_if_new(NODE, string("Predicate"));
    string concept_node_h = this->add_atom_if_new(NODE, string("Concept"));
    string evaluation_node_h = this->add_atom_if_new(NODE, string("Evaluation"));
    string literal_pk_node_h = this->add_atom_if_new(NODE, string(literal_pk));
    string table_name_node_h = this->add_atom_if_new(NODE, string(table_name));
    string fk_table_node_h = this->add_atom_if_new(NODE, string(fk_table));
    string literal_value_node_h = this->add_atom_if_new(NODE, string(literal_value));
    string fk_column_node_h = this->add_atom_if_new(NODE, string(fk_column));

    // Links
    string predicate_inner_1_link_h =
        this->add_atom_if_new(LINK,
                              vector<string>{fk_table_node_h, literal_value_node_h},
                              "(" + fk_table + " " + literal_value + ")");
    string predicate_inner_2_link_h =
        this->add_atom_if_new(LINK,
                              vector<string>{concept_node_h, predicate_inner_1_link_h},
                              "(Concept (" + fk_table + " " + literal_value + "))");
    string predicate_inner_3_link_h =
        this->add_atom_if_new(LINK,
                              vector<string>{fk_column_node_h, predicate_inner_2_link_h},
                              "(" + fk_column + " (Concept (" + fk_table + " " + literal_value + ")))");
    string predicate_link_h = this->add_atom_if_new(
        LINK,
        vector<string>{predicate_node_h, predicate_inner_3_link_h},
        "(Predicate (" + fk_column + " (Concept (" + fk_table + " " + literal_value + "))))");
    string concept_inner_link_h =
        this->add_atom_if_new(LINK,
                              vector<string>{table_name_node_h, literal_pk_node_h},
                              "(" + table_name + " " + literal_pk + ")");
    string concept_link_h = this->add_atom_if_new(LINK,
                                                  vector<string>{concept_node_h, concept_inner_link_h},
                                                  "(Concept (" + table_name + " " + literal_pk + "))");

    this->add_atom_if_new(LINK,
                          vector<string>{evaluation_node_h, predicate_link_h, concept_link_h},
                          "(Evaluation (Predicate (" + fk_column + " (Concept (" + fk_table + " " +
                              literal_value + ")))) (Concept (" + table_name + " " + literal_pk + ")))",
                          true);
}

void SQL2AtomsMapper::map_regular_column(const string& table_name,
                                         const string& column_name,
                                         const string& value,
                                         const string& primary_key_value) {
    string literal_pk = this->escape_inner_quotes("\"" + primary_key_value + "\"");
    string literal_value = this->escape_inner_quotes("\"" + value + "\"");

    // Nodes
    string predicate_node_h = this->add_atom_if_new(NODE, string("Predicate"));
    string concept_node_h = this->add_atom_if_new(NODE, string("Concept"));
    string evaluation_node_h = this->add_atom_if_new(NODE, string("Evaluation"));
    string table_name_node_h = this->add_atom_if_new(NODE, string(table_name));
    string column_name_node_h = this->add_atom_if_new(NODE, string(column_name));
    string literal_value_node_h = this->add_atom_if_new(NODE, string(literal_value));
    string literal_pk_node_h = this->add_atom_if_new(NODE, string(literal_pk));

    // Links
    string predicate_inner_link_h = this->add_atom_if_new(
        LINK,
        vector<string>{table_name_node_h, column_name_node_h, literal_value_node_h},
        "(" + table_name + " " + column_name + " " + literal_value + ")");
    string predicate_link_h = this->add_atom_if_new(
        LINK,
        vector<string>{predicate_node_h, predicate_inner_link_h},
        "(Predicate (" + table_name + " " + column_name + " " + literal_value + "))");
    string concept_inner_link_h =
        this->add_atom_if_new(LINK,
                              vector<string>{table_name_node_h, literal_pk_node_h},
                              "(" + table_name + " " + literal_pk + ")");
    string concept_link_h = this->add_atom_if_new(LINK,
                                                  vector<string>{concept_node_h, concept_inner_link_h},
                                                  "(Concept (" + table_name + " " + literal_pk + "))");
    this->add_atom_if_new(LINK,
                          vector<string>{evaluation_node_h, predicate_link_h, concept_link_h},
                          "(Evaluation (Predicate (" + table_name + " " + column_name + " " +
                              literal_value + ")) (Concept (" + table_name + " " + literal_pk + ")))",
                          true);
}

void SQL2AtomsMapper::map_foreign_keys_combinations(
    const vector<tuple<string, string, string>>& all_foreign_keys) {
    for (const auto& [column_name, foreign_table_name, value] : all_foreign_keys) {
        for (const auto& [column_name2, foreign_table_name2, value2] : all_foreign_keys) {
            if (make_pair(foreign_table_name, value) != make_pair(foreign_table_name2, value2)) {
                string literal_value = this->escape_inner_quotes("\"" + value + "\"");
                string literal_value_2 = this->escape_inner_quotes("\"" + value2 + "\"");

                // Nodes
                string predicate_node_h = this->add_atom_if_new(NODE, string("Predicate"));
                string concept_node_h = this->add_atom_if_new(NODE, string("Concept"));
                string evaluation_node_h = this->add_atom_if_new(NODE, string("Evaluation"));
                string fk_column_node_h = this->add_atom_if_new(NODE, string(column_name));
                string foreign_table_name_node_h =
                    this->add_atom_if_new(NODE, string(foreign_table_name));
                string literal_value_node_h = this->add_atom_if_new(NODE, string(literal_value));
                string foreign_table_name2_node_h =
                    this->add_atom_if_new(NODE, string(foreign_table_name2));
                string literal_value2_node_h = this->add_atom_if_new(NODE, string(literal_value_2));
                // Links
                string predicate_inner_1_link_h = this->add_atom_if_new(
                    LINK,
                    vector<string>{foreign_table_name_node_h, literal_value_node_h},
                    "(" + foreign_table_name + " " + literal_value + ")");
                string predicate_inner_2_link_h = this->add_atom_if_new(
                    LINK,
                    vector<string>{concept_node_h, predicate_inner_1_link_h},
                    "(Concept (" + foreign_table_name + " " + literal_value + "))");
                string predicate_inner_3_link_h =
                    this->add_atom_if_new(LINK,
                                          vector<string>{fk_column_node_h, predicate_inner_2_link_h},
                                          "(" + column_name + " (Concept (" + foreign_table_name + " " +
                                              literal_value + ")))");
                string predicate_link_h =
                    this->add_atom_if_new(LINK,
                                          vector<string>{predicate_node_h, predicate_inner_3_link_h},
                                          "(Predicate (" + column_name + " (Concept (" +
                                              foreign_table_name + " " + literal_value + "))))");
                string concept_inner_link_h = this->add_atom_if_new(
                    LINK,
                    vector<string>{foreign_table_name2_node_h, literal_value2_node_h},
                    "(" + foreign_table_name2 + " " + literal_value_2 + ")");
                string concept_link_h = this->add_atom_if_new(
                    LINK,
                    vector<string>{concept_node_h, concept_inner_link_h},
                    "(Concept (" + foreign_table_name2 + " " + literal_value_2 + "))");

                this->add_atom_if_new(
                    LINK,
                    vector<string>{evaluation_node_h, predicate_link_h, concept_link_h},
                    "(Evaluation (Predicate (" + column_name + " (Concept (" + foreign_table_name + " " +
                        literal_value + ")))) (Concept (" + foreign_table_name2 + " " + literal_value_2 +
                        ")))",
                    true);
            }
        }
    }
}

// ==============================
//  Construction / destruction
// ==============================

Metta2AtomsMapper::Metta2AtomsMapper() {
    RAISE_ERROR("Metta2AtomsMapper constructor not implemented yet");
}

Metta2AtomsMapper::~Metta2AtomsMapper() {}

// ==============================
//  Public
// ==============================

const vector<Atom*> Metta2AtomsMapper::map(const DbInput& data) {
    RAISE_ERROR("Metta2AtomsMapper::map() not implemented yet");
    return vector<Atom*>{};
}