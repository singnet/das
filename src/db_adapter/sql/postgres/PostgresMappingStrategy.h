#pragma once

#include <memory>
#include <vector>

#include "BoundedSharedQueue.h"
#include "DatabaseMappingStrategy.h"
#include "DatabaseTypes.h"
#include "JsonConfig.h"
#include "PostgresWrapper.h"

using namespace std;

namespace db_adapter {

class PostgresMappingStrategy : public DatabaseMappingStrategy {
   public:
    PostgresMappingStrategy(const JsonConfig& config,
                            shared_ptr<PostgresConnection> conn,
                            shared_ptr<BoundedSharedQueue> queue);

    vector<MappingTask> create_tasks() override;
    void execute_task(const MappingTask& task) override;

   private:
    const JsonConfig& config;
    shared_ptr<PostgresWrapper> wrapper;
};

}  // namespace db_adapter
