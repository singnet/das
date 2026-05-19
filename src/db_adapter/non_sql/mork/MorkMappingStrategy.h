#pragma once

#include <memory>
#include <string>
#include <vector>

#include "BoundedSharedQueue.h"
#include "DatabaseMappingStrategy.h"
#include "DatabaseTypes.h"
#include "JsonConfig.h"
#include "MorkConnection.h"
#include "MorkWrapper.h"

using namespace std;
using namespace commons;

namespace db_adapter {

class MorkMappingStrategy : public DatabaseMappingStrategy {
   public:
    MorkMappingStrategy(const JsonConfig& config,
                        shared_ptr<MorkConnection> conn,
                        shared_ptr<BoundedSharedQueue> queue);
    vector<MappingTask> create_tasks() override;
    void execute_task(const MappingTask& task) override;

   private:
    const JsonConfig& config;
    shared_ptr<MorkWrapper> wrapper;
};

}  // namespace db_adapter