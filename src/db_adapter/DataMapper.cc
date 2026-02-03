#include "DataMapper.h"

#include <iostream>
#include <variant>

#include "Atom.h"
#include "DataTypes.h"
#include "Hasher.h"
#include "Link.h"
#include "Logger.h"
#include "MettaMapping.h"
#include "Node.h"

using namespace db_adapter;
using namespace commons;
using namespace atoms;

string BaseSQL2Mapper::SYMBOL;
string BaseSQL2Mapper::EXPRESSION;

// -- BaseSQL2Mapper

const OutputList BaseSQL2Mapper::map(const DbInput& data) {
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
                Utils::error("Invalid foreign key format: " + column_name);
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
    return this->get_output();
}

bool BaseSQL2Mapper::is_foreign_key(const string& column_name) {
    size_t pos = column_name.find('|');
    if (pos == string::npos) return false;
    return true;
}

string BaseSQL2Mapper::escape_inner_quotes(string text) {
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

bool BaseSQL2Mapper::insert_handle_if_missing(const string& handle) {
    auto exists = this->handle_trie.exists(handle);
    if (exists) return false;
    this->handle_trie.insert(handle, NULL);
    return true;
}

// -- SQL2MettaMapper

SQL2MettaMapper::SQL2MettaMapper() { this->initialize_statics(); }

SQL2MettaMapper::~SQL2MettaMapper() {}

OutputList SQL2MettaMapper::get_output() { return this->metta_expressions; }

void SQL2MettaMapper::add_metta_if_new(const string& s_expression) {
    string key = Hasher::context_handle(s_expression);
    if (this->insert_handle_if_missing(key)) {
        this->metta_expressions.push_back(s_expression);
    }
};

void SQL2MettaMapper::map_primary_key(const string& table_name, const string& primary_key_value) {
    string literal_pk = this->escape_inner_quotes("\"" + primary_key_value + "\"");

    string predicate_link = "(Predicate " + table_name + ")";
    this->add_metta_if_new(predicate_link);

    string concept_inner_link = "(" + table_name + literal_pk + ")";
    this->add_metta_if_new(concept_inner_link);

    string concept_link = "(Concept " + concept_inner_link + ")";
    this->add_metta_if_new(concept_link);

    string evaluation_link = "(Evaluation " + predicate_link + " " + concept_link + ")";
    this->add_metta_if_new(evaluation_link);
}

void SQL2MettaMapper::map_foreign_key_column(const string& table_name,
                                             const string& column_name,
                                             const string& value,
                                             const string& primary_key_value,
                                             const string& fk_table,
                                             const string& fk_column) {
    string literal_value = this->escape_inner_quotes("\"" + value + "\"");
    string literal_pk = this->escape_inner_quotes("\"" + primary_key_value + "\"");

    string predicate_inner_1_link = "(" + fk_table + " " + literal_value + ")";
    this->add_metta_if_new(predicate_inner_1_link);

    string predicate_inner_2_link = "(Concept " + predicate_inner_1_link + ")";
    this->add_metta_if_new(predicate_inner_2_link);

    string predicate_inner_3_link = "(" + fk_column + " " + predicate_inner_2_link + ")";
    this->add_metta_if_new(predicate_inner_3_link);

    string predicate_link = "(Predicate " + predicate_inner_3_link + ")";
    this->add_metta_if_new(predicate_link);

    string concept_inner_link = "(" + table_name + " " + literal_pk + ")";
    this->add_metta_if_new(concept_inner_link);

    string concept_link = "(Concept " + concept_inner_link + ")";
    this->add_metta_if_new(concept_link);

    string evaluation_link = "(Evaluation " + predicate_link + " " + concept_link + ")";
    this->add_metta_if_new(evaluation_link);
}

void SQL2MettaMapper::map_regular_column(const string& table_name,
                                         const string& column_name,
                                         const string& value,
                                         const string& primary_key_value) {
    string literal_value = this->escape_inner_quotes("\"" + value + "\"");
    string literal_pk = this->escape_inner_quotes("\"" + primary_key_value + "\"");

    string predicate_inner_link = "(" + table_name + " " + column_name + " " + literal_value + ")";
    this->add_metta_if_new(predicate_inner_link);

    string predicate_link = "(Predicate " + predicate_inner_link + ")";
    this->add_metta_if_new(predicate_link);

    string concept_inner_link = "(" + table_name + " " + literal_pk + ")";
    this->add_metta_if_new(concept_inner_link);

    string concept_link = "(Concept " + concept_inner_link + ")";
    this->add_metta_if_new(concept_link);

    string evaluation_link = "(Evaluation " + predicate_link + " " + concept_link + ")";
    this->add_metta_if_new(evaluation_link);
}

void SQL2MettaMapper::map_foreign_keys_combinations(
    const vector<tuple<string, string, string>>& all_foreign_keys) {
    for (const auto& [column_name, foreign_table_name, value] : all_foreign_keys) {
        for (const auto& [column_name2, foreign_table_name2, value2] : all_foreign_keys) {
            if ((foreign_table_name != foreign_table_name2) && (value != value2)) {
                string literal_value = this->escape_inner_quotes("\"" + value + "\"");
                string literal_value_2 = this->escape_inner_quotes("\"" + value2 + "\"");

                string predicate_inner_1_link = "(" + foreign_table_name + " " + literal_value + ")";
                this->add_metta_if_new(predicate_inner_1_link);

                string predicate_inner_2_link = "(Concept " + predicate_inner_1_link + ")";
                this->add_metta_if_new(predicate_inner_2_link);

                string predicate_inner_3_link = "(" + column_name + " " + predicate_inner_2_link + ")";
                this->add_metta_if_new(predicate_inner_3_link);

                string predicate_link = "(Predicate " + predicate_inner_3_link + ")";
                this->add_metta_if_new(predicate_link);

                string concept_inner_link = "(" + foreign_table_name2 + " " + literal_value_2 + ")";
                this->add_metta_if_new(concept_inner_link);

                string concept_link = "(Concept " + concept_inner_link + ")";
                this->add_metta_if_new(concept_link);

                string evaluation_link = "(Evaluation " + predicate_link + " " + concept_link + ")";
                this->add_metta_if_new(evaluation_link);
            }
        }
    }
}

// -- SQL2AtomsMapper

SQL2AtomsMapper::SQL2AtomsMapper() { this->initialize_statics(); }

SQL2AtomsMapper::~SQL2AtomsMapper() {
    for (auto atom : this->atoms) delete atom;
}

OutputList SQL2AtomsMapper::get_output() { return this->atoms; }

string SQL2AtomsMapper::add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE atom_type,
                                        variant<string, vector<string>> value,
                                        bool is_toplevel) {
    Atom* atom;

    if (atom_type == SQL2AtomsMapper::ATOM_TYPE::NODE) {
        string name = get<string>(value);
        atom = new Node(BaseSQL2Mapper::SYMBOL, name);
    } else if (SQL2AtomsMapper::ATOM_TYPE::LINK) {
        vector<string> targets = get<vector<string>>(value);
        atom = new Link(BaseSQL2Mapper::EXPRESSION, targets, is_toplevel);
    } else {
        Utils::error("Either name or targets must be provided to create an Atom.");
    }

    string handle = atom->handle();
    if (this->insert_handle_if_missing(handle)) {
        this->atoms.push_back(atom);
    } else {
        delete atom;
    }
    return handle;
};

void SQL2AtomsMapper::map_primary_key(const string& table_name, const string& primary_key_value) {
    string literal_pk = this->escape_inner_quotes("\"" + primary_key_value + "\"");

    // Nodes
    string predicate_node_h =
        this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::NODE, string("Predicate"));
    string concept_node_h = this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::NODE, string("Concept"));
    string evaluation_node_h =
        this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::NODE, string("Evaluation"));
    string literal_pk_node_h =
        this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::NODE, string(literal_pk));
    string table_name_node_h =
        this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::NODE, string(table_name));

    // Links
    string predicate_link_h = this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::LINK,
                                                    vector<string>{predicate_node_h, table_name_node_h});
    string concept_inner_link_h = this->add_atom_if_new(
        SQL2AtomsMapper::ATOM_TYPE::LINK, vector<string>{table_name_node_h, literal_pk_node_h});
    string concept_link_h = this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::LINK,
                                                  vector<string>{concept_node_h, concept_inner_link_h});
    this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::LINK,
                          vector<string>{evaluation_node_h, predicate_link_h, concept_link_h},
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
    string predicate_node_h =
        this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::NODE, string("Predicate"));
    string concept_node_h = this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::NODE, string("Concept"));
    string evaluation_node_h =
        this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::NODE, string("Evaluation"));
    string literal_pk_node_h =
        this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::NODE, string(literal_pk));
    string table_name_node_h =
        this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::NODE, string(table_name));
    string fk_table_node_h = this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::NODE, string(fk_table));
    string literal_value_node_h =
        this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::NODE, string(literal_value));
    string fk_column_node_h = this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::NODE, string(fk_column));

    // Links
    string predicate_inner_1_link_h = this->add_atom_if_new(
        SQL2AtomsMapper::ATOM_TYPE::LINK, vector<string>{fk_table_node_h, literal_value_node_h});
    string predicate_inner_2_link_h = this->add_atom_if_new(
        SQL2AtomsMapper::ATOM_TYPE::LINK, vector<string>{concept_node_h, predicate_inner_1_link_h});
    string predicate_inner_3_link_h = this->add_atom_if_new(
        SQL2AtomsMapper::ATOM_TYPE::LINK, vector<string>{fk_column_node_h, predicate_inner_2_link_h});
    string predicate_link_h = this->add_atom_if_new(
        SQL2AtomsMapper::ATOM_TYPE::LINK, vector<string>{predicate_node_h, predicate_inner_3_link_h});
    string concept_inner_link_h = this->add_atom_if_new(
        SQL2AtomsMapper::ATOM_TYPE::LINK, vector<string>{table_name_node_h, literal_pk_node_h});
    string concept_link_h = this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::LINK,
                                                  vector<string>{concept_node_h, concept_inner_link_h});

    this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::LINK,
                          vector<string>{evaluation_node_h, predicate_link_h, concept_link_h},
                          true);
}

void SQL2AtomsMapper::map_regular_column(const string& table_name,
                                         const string& column_name,
                                         const string& value,
                                         const string& primary_key_value) {
    string literal_pk = this->escape_inner_quotes("\"" + primary_key_value + "\"");
    string literal_value = this->escape_inner_quotes("\"" + value + "\"");

    // Nodes
    string predicate_node_h =
        this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::NODE, string("Predicate"));
    string concept_node_h = this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::NODE, string("Concept"));
    string evaluation_node_h =
        this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::NODE, string("Evaluation"));
    string table_name_node_h =
        this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::NODE, string(table_name));
    string column_name_node_h =
        this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::NODE, string(column_name));
    string literal_value_node_h =
        this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::NODE, string(literal_value));
    string literal_pk_node_h =
        this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::NODE, string(literal_pk));

    // Links
    string predicate_inner_link_h = this->add_atom_if_new(
        SQL2AtomsMapper::ATOM_TYPE::LINK,
        vector<string>{table_name_node_h, column_name_node_h, literal_value_node_h});
    string predicate_link_h = this->add_atom_if_new(
        SQL2AtomsMapper::ATOM_TYPE::LINK, vector<string>{predicate_node_h, predicate_inner_link_h});
    string concept_inner_link_h = this->add_atom_if_new(
        SQL2AtomsMapper::ATOM_TYPE::LINK, vector<string>{table_name_node_h, literal_pk_node_h});
    string concept_link_h = this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::LINK,
                                                  vector<string>{concept_node_h, concept_inner_link_h});
    this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::LINK,
                          vector<string>{evaluation_node_h, predicate_link_h, concept_link_h},
                          true);
}

void SQL2AtomsMapper::map_foreign_keys_combinations(
    const vector<tuple<string, string, string>>& all_foreign_keys) {
    for (const auto& [column_name, foreign_table_name, value] : all_foreign_keys) {
        for (const auto& [column_name2, foreign_table_name2, value2] : all_foreign_keys) {
            if ((foreign_table_name != foreign_table_name2) && (value != value2)) {
                string literal_value = this->escape_inner_quotes("\"" + value + "\"");
                string literal_value_2 = this->escape_inner_quotes("\"" + value2 + "\"");

                // Nodes
                string predicate_node_h =
                    this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::NODE, string("Predicate"));
                string concept_node_h =
                    this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::NODE, string("Concept"));
                string evaluation_node_h =
                    this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::NODE, string("Evaluation"));
                string fk_column_node_h =
                    this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::NODE, string(column_name));
                string foreign_table_name_node_h =
                    this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::NODE, string(foreign_table_name));
                string literal_value_node_h =
                    this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::NODE, string(literal_value));
                string foreign_table_name2_node_h =
                    this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::NODE, string(foreign_table_name2));
                string literal_value2_node_h =
                    this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::NODE, string(literal_value_2));
                // Links
                string predicate_inner_1_link_h = this->add_atom_if_new(
                    SQL2AtomsMapper::ATOM_TYPE::LINK,
                    vector<string>{foreign_table_name_node_h, literal_value_node_h});
                string predicate_inner_2_link_h =
                    this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::LINK,
                                          vector<string>{concept_node_h, predicate_inner_1_link_h});
                string predicate_inner_3_link_h =
                    this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::LINK,
                                          vector<string>{fk_column_node_h, predicate_inner_2_link_h});
                string predicate_link_h =
                    this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::LINK,
                                          vector<string>{predicate_node_h, predicate_inner_3_link_h});
                string concept_inner_link_h = this->add_atom_if_new(
                    SQL2AtomsMapper::ATOM_TYPE::LINK,
                    vector<string>{foreign_table_name2_node_h, literal_value2_node_h});
                string concept_link_h =
                    this->add_atom_if_new(SQL2AtomsMapper::ATOM_TYPE::LINK,
                                          vector<string>{concept_node_h, concept_inner_link_h});

                this->add_atom_if_new(
                    SQL2AtomsMapper::ATOM_TYPE::LINK,
                    vector<string>{evaluation_node_h, predicate_link_h, concept_link_h},
                    true);
            }
        }
    }
}
