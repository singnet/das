#include "DataMapper.h"
#include "DataTypes.h"

using namespace db_adapter;
using namespace commons;

db_adapter::SQL2MettaMapper::SQL2MettaMapper() {
    this->metta_expressions = vector<string>();
}

db_adapter::SQL2MettaMapper::~SQL2MettaMapper() {}

const db_adapter_types::OutputList SQL2MettaMapper::map(db_adapter_types::DbInput& data) {
    vector<tuple<string, string, string>> all_foreing_keys;
    db_adapter_types::SqlRow sql_row = get<db_adapter_types::SqlRow>(data);
    string table_name = sql_row.table_name;

    string primary_key_value = sql_row.primary_key ? sql_row.primary_key->value : "";

    this->process_primary_key(table_name, primary_key_value);

    for (auto field : sql_row.fields) {
        // pulic.feature.type_id|public.cvterm
        string column_name = table_name + "." + field.name;
        if (this->is_foreign_key(column_name)) {
            this->process_foreign_key_column(table_name, column_name, field.value, primary_key_value, all_foreing_keys);
        } else {
            this->process_regular_column(table_name, column_name, field.value, primary_key_value);
        };

    }

    // this->process_foreign_keys_combinations(all_foreing_keys);

    return this->metta_expressions;
}

bool SQL2MettaMapper::is_foreign_key(string& column_name) {
    size_t pos = column_name.find('|');
    if (pos == string::npos) { return false; }    
    return true;
}

string db_adapter::SQL2MettaMapper::escape_inner_quotes(string& text) {
    const auto count = std::count(text.begin(), text.end(), '"');

    if (count == 0 || count == 2) {
        return text;
    }

    if (count == 1) {
        text.insert(text.begin(), '"');
        text.push_back('"');
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

    return text;
}

void SQL2MettaMapper::process_primary_key(string& table_name, string& primary_key_value) {
    string predicate_link = "(Predicate " + table_name + ")";
    this->metta_expressions.push_back(predicate_link);

    string value = this->escape_inner_quotes('"' + primary_key_value + '"');
    string concept_inner_link = "(" + table_name + value + ")";
    this->metta_expressions.push_back(concept_inner_link);

    string concept_link = "(Concept " + concept_inner_link + ")";
    this->metta_expressions.push_back(concept_link);

    string evaluation_link = "(Evaluation " + predicate_link + concept_link +  ")";
    this->metta_expressions.push_back(evaluation_link);
}

void db_adapter::SQL2MettaMapper::process_foreign_key_column(
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

    string literal_value = this->escape_inner_quotes('"' + value + '"');
    string predicate_inner_1_link = "(" + fk_tavle + " " + literal_value + ")";
    this->metta_expression.push_back(predicate_inner_1_link);

    string predicate_inner_2_link = "(Concept " + predicate_inner_1_link + ")";
    this->metta_expression.push_back(predicate_inner_2_link);

    string predicate_inner_3_link = "(" + fk_column + " " + predicate_inner_2_link + ")";
    this->metta_expressions.push_back(predicate_inner_3_link);
}

// def _process_foreign_key(
//     self,
//     table_name: str,
//     column_name: str,
//     value: str,
//     primary_key_value: str,
// ) -> None::
//     column_name, foreign_table_name = column_name.split('|')
//     expr_inner_1 = "({foreign_table_name} {value})"
//     self.mapped_expressions.append(
//         self._process_metta_expression(
//             expr_inner_1, foreign_table_name=foreign_table_name, value=value
//         )
//     )
//     expr_inner_2 = "(Concept ({foreign_table_name} {value}))"
//     self.mapped_expressions.append(
//         self._process_metta_expression(
//             expr_inner_2,
//             foreign_table_name=foreign_table_name,
//             value=value,
//         )
//     )
//     expr_inner_3 = "({column_name} (Concept ({foreign_table_name} {value})))"
//     self.mapped_expressions.append(
//         self._process_metta_expression(
//             expr_inner_3,
//             column_name=column_name,
//             foreign_table_name=foreign_table_name,
//             value=value,
//         )
//     )
//     expr_inner_4 = "(Predicate ({column_name} (Concept ({foreign_table_name} {value}))))"
//     self.mapped_expressions.append(
//         self._process_metta_expression(
//             expr_inner_4,
//             column_name=column_name,
//             foreign_table_name=foreign_table_name,
//             value=value,
//             table_name=table_name,
//             primary_key_value=primary_key_value,
//         )
//     )
//     expr_inner_5 = "({table_name} {primary_key_value})"
//     self.mapped_expressions.append(
//         self._process_metta_expression(
//             expr_inner_5,
//             table_name=table_name,
//             primary_key_value=primary_key_value,
//         )
//     )
//     expr_inner_6 = "(Concept ({table_name} {primary_key_value}))"
//     self.mapped_expressions.append(
//         self._process_metta_expression(
//             expr_inner_6,
//             table_name=table_name,
//             primary_key_value=primary_key_value,
//         )
//     )
//     expr = "(Evaluation (Predicate ({column_name} (Concept ({foreign_table_name} {value})))) (Concept ({table_name} {primary_key_value})))"
//     self.mapped_expressions.append(
//         self._process_metta_expression(
//             expr,
//             column_name=column_name,
//             foreign_table_name=foreign_table_name,
//             value=value,
//             table_name=table_name,
//             primary_key_value=primary_key_value,
//         )
//     )
