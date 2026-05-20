#pragma once

#include <memory>
#include <vector>

#include "DatabaseConnection.h"
#include "DatabaseTypes.h"

using namespace std;

namespace db_adapter {

class DatabaseMappingStrategy {
   public:
    explicit DatabaseMappingStrategy(shared_ptr<DatabaseConnection> conn);
    virtual ~DatabaseMappingStrategy() = default;

    void start_connection();
    void stop_connection();

    virtual vector<MappingTask> create_tasks() = 0;
    virtual void execute_task(const MappingTask& task) = 0;

   private:
    shared_ptr<DatabaseConnection> conn;
};

}  // namespace db_adapter
