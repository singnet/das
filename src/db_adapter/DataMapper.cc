#include "DataMapper.h"

#include "Atom.h"
#include "DataTypes.h"
#include "Link.h"
#include "MettaMapping.h"
#include "Node.h"

using namespace db_adapter;
using namespace commons;
using namespace atoms;

string BaseSQL2Mapper::SYMBOL;
string BaseSQL2Mapper::EXPRESSION;

// -- BaseSQL2Mapper

const db_adapter_types::OutputList BaseSQL2Mapper::map(db_adapter_types::DbInput& data) {
    vector<tuple<string, string, string>> all_foreing_keys;
    db_adapter_types::SqlRow sql_row = get<db_adapter_types::SqlRow>(data);
    string table_name = sql_row.table_name;

    string primary_key_value = sql_row.primary_key ? sql_row.primary_key->value : "";

    this->process_primary_key(table_name, primary_key_value);

    for (auto field : sql_row.fields) {
        string column_name = table_name + "." + field.name;
        if (this->is_foreign_key(column_name)) {
            this->process_foreign_key_column(
                table_name, column_name, field.value, primary_key_value, all_foreing_keys);
        } else {
            this->process_regular_column(table_name, column_name, field.value, primary_key_value);
        };
    }

    this->process_foreign_keys_combinations(all_foreing_keys);

    return this->get_output();
}

bool BaseSQL2Mapper::is_foreign_key(string& column_name) {
    size_t pos = column_name.find('|');
    if (pos == string::npos) {
        return false;
    }
    return true;
}

string BaseSQL2Mapper::escape_inner_quotes(string text) {
    const auto count = std::count(text.begin(), text.end(), '"');

    if (count == 0 || count == 2) {
        return text;
    }

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
}

bool db_adapter::BaseSQL2Mapper::insert_handle_if_missing(const string& handle) {
    MapperValue* m_value;
    m_value = (MapperValue*) this->handle_trie.lookup(handle);

    if (m_value != NULL) return false;

    this->handle_trie.insert(handle, this->default_mapper_value);
    return true;
}

// -- SQL2MettaMapper

SQL2MettaMapper::SQL2MettaMapper() {
   this->default_mapper_value = new MapperValue();
}

SQL2MettaMapper::~SQL2MettaMapper() {}

db_adapter_types::OutputList db_adapter::SQL2MettaMapper::get_output() {
    return this->metta_expressions;
}

void SQL2MettaMapper::process_primary_key(string& table_name, string& primary_key_value) {
    string literal_pk = this->escape_inner_quotes("\"" + primary_key_value + "\"");

    string predicate_link = "(Predicate " + table_name + ")";
    this->metta_expressions.push_back(predicate_link);

    string concept_inner_link = "(" + table_name + literal_pk + ")";
    this->metta_expressions.push_back(concept_inner_link);

    string concept_link = "(Concept " + concept_inner_link + ")";
    this->metta_expressions.push_back(concept_link);

    string evaluation_link = "(Evaluation " + predicate_link + " " + concept_link + ")";
    this->metta_expressions.push_back(evaluation_link);
}

void SQL2MettaMapper::process_foreign_key_column(
    string& table_name,
    string& column_name,
    string& value,
    string& primary_key_value,
    vector<tuple<string, string, string>>& all_foreign_keys) {
    size_t separator_pos = column_name.find('|');
    if (separator_pos == string::npos) {
        Utils::error("Invalid foreign key format: " + column_name);
    }
    string fk_column = column_name.substr(0, separator_pos);
    string fk_table = column_name.substr(separator_pos + 1);
    all_foreign_keys.push_back({fk_column, fk_table, value});

    string literal_value = this->escape_inner_quotes("\"" + value + "\"");
    string literal_pk = this->escape_inner_quotes("\"" + primary_key_value + "\"");

    string predicate_inner_1_link = "(" + fk_table + " " + literal_value + ")";
    this->metta_expressions.push_back(predicate_inner_1_link);

    string predicate_inner_2_link = "(Concept " + predicate_inner_1_link + ")";
    this->metta_expressions.push_back(predicate_inner_2_link);

    string predicate_inner_3_link = "(" + fk_column + " " + predicate_inner_2_link + ")";
    this->metta_expressions.push_back(predicate_inner_3_link);

    string predicate_link = "(Predicate " + predicate_inner_3_link + ")";
    this->metta_expressions.push_back(predicate_link);

    string concept_inner_link = "(" + table_name + " " + literal_pk + ")";
    this->metta_expressions.push_back(concept_inner_link);

    string concept_link = "(Concept " + concept_inner_link + ")";
    this->metta_expressions.push_back(concept_link);

    string evaluation_link = "(Evaluation " + predicate_link + " " + concept_link + ")";
    this->metta_expressions.push_back(evaluation_link);
}

void SQL2MettaMapper::process_regular_column(string& table_name,
                                             string& column_name,
                                             string& value,
                                             string& primary_key_value) {
    string literal_value = this->escape_inner_quotes("\"" + value + "\"");
    string literal_pk = this->escape_inner_quotes("\"" + primary_key_value + "\"");

    string predicate_inner_link = "(" + table_name + " " + column_name + " " + literal_value + ")";
    this->metta_expressions.push_back(predicate_inner_link);

    string predicate_link = "(Predicate " + predicate_inner_link + ")";
    this->metta_expressions.push_back(predicate_link);

    string concept_inner_link = "(" + table_name + " " + literal_pk + ")";
    this->metta_expressions.push_back(concept_inner_link);

    string concept_link = "(Concept " + concept_inner_link + ")";
    this->metta_expressions.push_back(concept_link);

    string evaluation_link = "(Evaluation " + predicate_link + " " + concept_link + ")";
    this->metta_expressions.push_back(evaluation_link);
}

void SQL2MettaMapper::process_foreign_keys_combinations(
    vector<tuple<string, string, string>> all_foreign_keys) {
    for (const auto& [column_name, foreign_table_name, value] : all_foreign_keys) {
        for (const auto& [column_name2, foreign_table_name2, value2] : all_foreign_keys) {
            if ((foreign_table_name != foreign_table_name2) && (value != value2)) {
                string literal_value = this->escape_inner_quotes("\"" + value + "\"");
                string literal_value_2 = this->escape_inner_quotes("\"" + value2 + "\"");

                string predicate_inner_1_link = "(" + foreign_table_name + " " + literal_value + ")";
                this->metta_expressions.push_back(predicate_inner_1_link);

                string predicate_inner_2_link = "(Concept " + predicate_inner_1_link + ")";
                this->metta_expressions.push_back(predicate_inner_2_link);

                string predicate_inner_3_link = "(" + column_name + " " + predicate_inner_2_link + ")";
                this->metta_expressions.push_back(predicate_inner_3_link);

                string predicate_link = "(Predicate " + predicate_inner_3_link + ")";
                this->metta_expressions.push_back(predicate_link);

                string concept_inner_link = "(" + foreign_table_name2 + " " + literal_value_2 + ")";
                this->metta_expressions.push_back(concept_inner_link);

                string concept_link = "(Concept " + concept_inner_link + ")";
                this->metta_expressions.push_back(concept_link);

                string evaluation_link = "(Evaluation " + predicate_link + " " + concept_link + ")";
                this->metta_expressions.push_back(evaluation_link);
            }
        }
    }
}

// -- SQL2AtomsMapper

SQL2AtomsMapper::SQL2AtomsMapper() {
    this->default_mapper_value = new MapperValue();
}

SQL2AtomsMapper::~SQL2AtomsMapper() {}

db_adapter_types::OutputList db_adapter::SQL2AtomsMapper::get_output() { return this->atoms; }

string SQL2AtomsMapper::add_atom_if_new(const string& name, const vector<string>& targets, bool is_toplevel) {
    Atom* atom;
    
    if (!name.empty()) {
        atom = new Node(BaseSQL2Mapper::SYMBOL, name);
    } else if (!targets.empty()) {
        atom = new Link(BaseSQL2Mapper::EXPRESSION, targets, is_toplevel);
    } else {
        Utils::error("Either name or targets must be provided to create an Atom.");
    }
    
    if (this->insert_handle_if_missing(atom->handle())) {
        this->atoms.push_back(atom);
        return atom->handle();
    } else {
        delete atom;
        return "";
    }
};

void SQL2AtomsMapper::process_primary_key(string& table_name, string& primary_key_value) {
    string literal_pk = this->escape_inner_quotes("\"" + primary_key_value + "\"");
   
    // Nodes
    string predicate_node_h = this->add_atom_if_new("Predicate", {});
    string concept_node_h = this->add_atom_if_new("Concept", {});
    string evaluation_node_h = this->add_atom_if_new("Evaluation", {});
    string literal_pk__node_h = this->add_atom_if_new(literal_pk, {});
    string table_name_node_h = this->add_atom_if_new(table_name, {});

    // Links
    string predicate_link_h = this->add_atom_if_new("", {predicate_node_h, table_name_node_h});
    string concept_inner_link_h = this->add_atom_if_new("", {table_name_node_h, literal_pk__node_h});
    string concept_link_h = this->add_atom_if_new("", {concept_node_h, concept_inner_link_h});
    
    this->add_atom_if_new("", {evaluation_node_h, predicate_link_h, concept_link_h}, true);

    // auto predicate_node = new Node(BaseSQL2Mapper::SYMBOL, "Predicate");
    // auto concept_node = new Node(BaseSQL2Mapper::SYMBOL, "Concept");
    // auto evaluation_node = new Node(BaseSQL2Mapper::SYMBOL, "Evaluation");
    // auto literal_pk_node = new Node(BaseSQL2Mapper::SYMBOL, literal_pk);
    // auto table_name_node = new Node(BaseSQL2Mapper::SYMBOL, table_name);
    // auto predicate_link = new Link(BaseSQL2Mapper::EXPRESSION, {predicate_node->handle(), table_name_node->handle()}, false);
    // auto concept_inner_link = new Link(BaseSQL2Mapper::EXPRESSION, {table_name_node->handle(), literal_pk_node->handle()}, false);
    // auto concept_link = new Link(BaseSQL2Mapper::EXPRESSION, {concept_node->handle(), concept_inner_link->handle()}, false);
    // auto evaluation_link =new Link(BaseSQL2Mapper::EXPRESSION, {evaluation_node->handle(), predicate_link->handle(), concept_link->handle()}, true);
}

void SQL2AtomsMapper::process_foreign_key_column(
    string& table_name,
    string& column_name,
    string& value,
    string& primary_key_value,
    vector<tuple<string, string, string>>& all_foreign_keys
) {
    size_t separator_pos = column_name.find('|');
    if (separator_pos == string::npos) {
        Utils::error("Invalid foreign key format: " + column_name);
    }
    string fk_column = column_name.substr(0, separator_pos);
    string fk_table = column_name.substr(separator_pos + 1);
    all_foreign_keys.push_back({fk_column, fk_table, value});

    string literal_value = this->escape_inner_quotes("\"" + value + "\"");
    string literal_pk = this->escape_inner_quotes("\"" + primary_key_value + "\"");

    // Nodes
    string predicate_node_h = this->add_atom_if_new("Predicate", {});
    string concept_node_h = this->add_atom_if_new("Concept", {});
    string evaluation_node_h = this->add_atom_if_new("Evaluation", {});
    string literal_pk_node_h = this->add_atom_if_new(literal_pk, {});
    string table_name_node_h = this->add_atom_if_new(table_name, {});
    string fk_table_node_h = this->add_atom_if_new(fk_table, {});
    string literal_value_node_h = this->add_atom_if_new(literal_value, {});
    string fk_column_node_h = this->add_atom_if_new(fk_column, {});

    // Links
    string predicate_inner_1_link_h = this->add_atom_if_new("", {fk_table_node_h, literal_value_node_h});
    string predicate_inner_2_link_h = this->add_atom_if_new("", {concept_node_h, predicate_inner_1_link_h});
    string predicate_inner_3_link_h = this->add_atom_if_new("", {fk_column_node_h, predicate_inner_2_link_h});
    string predicate_link_h = this->add_atom_if_new("", {predicate_node_h, predicate_inner_3_link_h});
    string concept_inner_link_h = this->add_atom_if_new("", {table_name_node_h, literal_pk_node_h});
    string concept_link_h = this->add_atom_if_new("", {concept_node_h, concept_inner_link_h});
    
    this->add_atom_if_new("", {evaluation_node_h, predicate_link_h, concept_link_h}, true);

    // auto predicate_node = new Node(BaseSQL2Mapper::SYMBOL, "Predicate");
    // auto concept_node = new Node(BaseSQL2Mapper::SYMBOL, "Concept");
    // auto evaluation_node = new Node(BaseSQL2Mapper::SYMBOL, "Evaluation");
    // auto literal_pk_node = new Node(BaseSQL2Mapper::SYMBOL, literal_pk);
    // auto table_name_node = new Node(BaseSQL2Mapper::SYMBOL, table_name);
    // auto fk_table_node = new Node(BaseSQL2Mapper::SYMBOL, fk_table);
    // auto literal_value_node = new Node(BaseSQL2Mapper::SYMBOL, literal_value);
    // auto fk_column_node = new Node(BaseSQL2Mapper::SYMBOL, fk_column);
    // auto predicate_inner_1_link = new Link(BaseSQL2Mapper::EXPRESSION, {fk_table_node->handle(), literal_value_node->handle()}, false);
    // auto predicate_inner_2_link = new Link(BaseSQL2Mapper::EXPRESSION, {concept_node->handle(), predicate_inner_1_link->handle()}, false);
    // auto predicate_inner_3_link = new Link(BaseSQL2Mapper::EXPRESSION, {fk_column_node->handle(), predicate_inner_2_link->handle()}, false);
    // auto predicate_link = new Link(BaseSQL2Mapper::EXPRESSION, {predicate_node->handle(), predicate_inner_3_link->handle()}, false);
    // auto concept_inner_link = new Link(BaseSQL2Mapper::EXPRESSION, {table_name_node->handle(), literal_pk_node->handle()}, false);
    // auto concept_link = new Link(BaseSQL2Mapper::EXPRESSION, {concept_node->handle(), concept_inner_link->handle()}, false);
    // auto evaluation_link = new Link(BaseSQL2Mapper::EXPRESSION,{evaluation_node->handle(), predicate_link->handle(), concept_link->handle()},true);

    // if (this->insert_handle_if_missing(predicate_node->handle())) this->atoms.push_back(predicate_node);
    // if (this->insert_handle_if_missing(concept_node->handle())) this->atoms.push_back(concept_node);
    // if (this->insert_handle_if_missing(evaluation_node->handle())) this->atoms.push_back(evaluation_node);
    // if (this->insert_handle_if_missing(literal_pk_node->handle())) this->atoms.push_back(literal_pk_node);
    // if (this->insert_handle_if_missing(table_name_node->handle())) this->atoms.push_back(table_name_node);
    // if (this->insert_handle_if_missing(fk_table_node->handle())) this->atoms.push_back(fk_table_node);
    // if (this->insert_handle_if_missing(literal_value_node->handle())) this->atoms.push_back(literal_value_node);
    // if (this->insert_handle_if_missing(fk_column_node->handle())) this->atoms.push_back(fk_column_node);
    // if (this->insert_handle_if_missing(predicate_inner_1_link->handle())) this->atoms.push_back(predicate_inner_1_link);
    // if (this->insert_handle_if_missing(predicate_inner_2_link->handle())) this->atoms.push_back(predicate_inner_2_link);
    // if (this->insert_handle_if_missing(predicate_inner_3_link->handle())) this->atoms.push_back(predicate_inner_3_link);
    // if (this->insert_handle_if_missing(predicate_link->handle())) this->atoms.push_back(predicate_link);
    // if (this->insert_handle_if_missing(concept_inner_link->handle())) this->atoms.push_back(concept_inner_link);
    // if (this->insert_handle_if_missing(concept_link->handle())) this->atoms.push_back(concept_link);
    // if (this->insert_handle_if_missing(evaluation_link->handle())) this->atoms.push_back(evaluation_link);
}

void SQL2AtomsMapper::process_regular_column(
    string& table_name,
    string& column_name,
    string& value,
    string& primary_key_value
) {
    string literal_pk = this->escape_inner_quotes("\"" + primary_key_value + "\"");
    string literal_value = this->escape_inner_quotes("\"" + value + "\"");
    
    // Nodes
    string predicate_node_h = this->add_atom_if_new("Predicate", {});
    string concept_node_h = this->add_atom_if_new("Concept", {});
    string evaluation_node_h = this->add_atom_if_new("Evaluation", {});
    string table_name_node_h = this->add_atom_if_new(table_name, {});
    string column_name_node_h = this->add_atom_if_new(column_name, {});
    string literal_value_node_h = this->add_atom_if_new(literal_value, {});
    string literal_pk__node_h = this->add_atom_if_new(literal_pk, {});
    
    // Links
    string predicate_inner_link_h = this->add_atom_if_new("", {table_name_node_h, column_name_node_h, literal_value_node_h});
    string predicate_link_h = this->add_atom_if_new("", {predicate_node_h, predicate_inner_link_h});
    string concept_inner_link_h = this->add_atom_if_new("", {table_name_node_h, literal_pk__node_h});
    string concept_link_h = this->add_atom_if_new("", {concept_node_h, concept_inner_link_h});

    this->add_atom_if_new("", {evaluation_node_h, predicate_link_h, concept_link_h}, true);
}

void SQL2AtomsMapper::process_foreign_keys_combinations(
    vector<tuple<string, string, string>> all_foreign_keys) {
    for (const auto& [column_name, foreign_table_name, value] : all_foreign_keys) {
        for (const auto& [column_name2, foreign_table_name2, value2] : all_foreign_keys) {
            if ((foreign_table_name != foreign_table_name2) && (value != value2)) {
                string literal_value = this->escape_inner_quotes("\"" + value + "\"");
                string literal_value_2 = this->escape_inner_quotes("\"" + value2 + "\"");
                
                // Nodes
                string predicate_node_h = this->add_atom_if_new("Predicate", {});
                string concept_node_h = this->add_atom_if_new("Concept", {});
                string evaluation_node_h = this->add_atom_if_new("Evaluation", {});
                string fk_column_node_h = this->add_atom_if_new(column_name, {});
                string foreign_table_name_node_h = this->add_atom_if_new(foreign_table_name, {});
                string literal_value_node_h = this->add_atom_if_new(literal_value, {});
                string foreign_table_name2_node_h = this->add_atom_if_new(foreign_table_name2, {});
                string literal_value2_node_h = this->add_atom_if_new(literal_value_2, {});

                // Links
                string predicate_inner_1_link_h = this->add_atom_if_new("", {foreign_table_name_node_h, literal_value_node_h});
                string predicate_inner_2_link_h = this->add_atom_if_new("", {concept_node_h, predicate_inner_1_link_h});
                string predicate_inner_3_link_h = this->add_atom_if_new("", {fk_column_node_h, predicate_inner_2_link_h});
                string predicate_link_h = this->add_atom_if_new("", {predicate_node_h, predicate_inner_3_link_h});
                string concept_inner_link_h = this->add_atom_if_new("", {foreign_table_name2_node_h, literal_value2_node_h});
                string concept_link_h = this->add_atom_if_new("", {concept_node_h, concept_inner_link_h});
                
                this->add_atom_if_new("", {evaluation_node_h, predicate_link_h, concept_link_h}, true);
            }
        }
    }
}
