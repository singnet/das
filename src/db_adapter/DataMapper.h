#pragma once

#include <string>
#include <algorithm>
#include "DataTypes.h"
using namespace std;

namespace db_adapter {
class Mapper{
   public:
    virtual ~Mapper() = default;
    virtual const db_adapter_types::OutputList map(db_adapter_types::DbInput& data) = 0;
   
   protected:
    Mapper() = default;
};

class SQL2MettaMapper : public Mapper {
   public:
    SQL2MettaMapper();
    ~SQL2MettaMapper() override;
    const db_adapter_types::OutputList map(db_adapter_types::DbInput& data) override;
   
    private:
    vector<string> metta_expressions;
    bool SQL2MettaMapper::is_foreign_key(string& column_name);
    string escape_inner_quotes(string& text);
    void process_primary_key(string& table_name, string& primary_key_value);
    void process_foreign_key_column(string& table_name, string& column_name, string& value, string& primary_key_value, vector<tuple<string, string, string>>& all_foreign_keys);
};
}  // namespace db_adapter