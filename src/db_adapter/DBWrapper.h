#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "DataMapper.h"

using namespace std;
using namespace db_adapter;

namespace db_adapter {
template <typename ConnT>
class DatabaseWrapper {
   public:
    explicit DatabaseWrapper(shared_ptr<Mapper> mapper, MAPPER_TYPE mapper_type)
        : mapper(move(mapper)), mapper_type(mapper_type) {}
    virtual ~DatabaseWrapper() = default;

    virtual unique_ptr<ConnT> connect() = 0;
    virtual void disconnect() = 0;

   protected:
    unique_ptr<ConnT> db_client;
    shared_ptr<Mapper> mapper;
    MAPPER_TYPE mapper_type;
};

template <typename ConnT>
class SQLWrapper : public DatabaseWrapper<ConnT> {
   public:
    explicit SQLWrapper(MAPPER_TYPE mapper_type)
        : DatabaseWrapper<ConnT>(create_mapper(mapper_type), mapper_type) {}
    virtual ~SQLWrapper() = default;

    virtual Table get_table(const string& name) = 0;
    virtual vector<Table> list_tables() = 0;
    virtual void map_table(const Table& table,
                           const vector<string>& clauses,
                           const vector<string>& skip_columns,
                           bool second_level) = 0;
    virtual void map_sql_query(const string& virtual_name, const string& raw_query) = 0;

   private:
    static shared_ptr<Mapper> create_mapper(MAPPER_TYPE mapper_type) {
        switch (mapper_type) {
            case MAPPER_TYPE::SQL2METTA:
                return make_shared<SQL2MettaMapper>();
            case MAPPER_TYPE::SQL2ATOMS:
                return make_shared<SQL2AtomsMapper>();
            default:
                throw invalid_argument("Unknown mapper type");
        }
    }
};
}  // namespace db_adapter