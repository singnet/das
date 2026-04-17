#pragma once

#include <memory>
#include <string>
#include <vector>

#include "DatabaseTypes.h"

using namespace std;

namespace db_adapter {

class DatabaseAdapterRunner {
   public:
    DatabaseAdapterRunner(const string& host,
                          int port,
                          const string& database,
                          const string& username,
                          const string& password,
                          const vector<TableMapping>& tables_mapping,
                          const vector<string>& queries_SQL,
                          MAPPER_TYPE mapper_type);

    void run();

   private:
    string host;
    int port;
    string database;
    string username;
    string password;
    vector<TableMapping> tables_mapping;
    vector<string> queries_SQL;
    MAPPER_TYPE mapper_type;
};

}  // namespace db_adapter