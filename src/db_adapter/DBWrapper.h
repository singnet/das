#pragma once
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "DataMapper.h"

using namespace std;
using namespace db_adapter;
using namespace db_adapter_types;

template <typename ConnT>
class DatabaseWrapper {
   public:
    explicit DatabaseWrapper(shared_ptr<Mapper> mapper);
    virtual ~DatabaseWrapper() = default;

    virtual unique_ptr<ConnT> connect() = 0;
    virtual void disconnect() = 0;

   protected:
    unique_ptr<ConnT> db_client;
    shared_ptr<Mapper> mapper;
};

template <typename ConnT>
class SQLWrapper : public DatabaseWrapper<ConnT> {
   public:
    explicit SQLWrapper(db_adapter_types::MAPPER_TYPE mapper_type);
    virtual ~SQLWrapper() = default;

    virtual shared_ptr<Table> get_table(const string& name) = 0;
    virtual vector<shared_ptr<Table>> list_tables() = 0;
    virtual void map_table(const Table& table,
                           const vector<string>& clauses,
                           const vector<string>* skip_columns,
                           bool second_level,
                           const function<void(const vector<Atom>&)>& on_row) = 0;

   private:
    static shared_ptr<Mapper> create_mapper(db_adapter_types::MAPPER_TYPE mapper_type);
};