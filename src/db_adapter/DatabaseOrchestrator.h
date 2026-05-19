#pragma once

#include <memory>
#include <vector>

#include "BoundedSharedQueue.h"
#include "DatabaseMappingStrategy.h"
#include "DatabaseTypes.h"
#include "DedicatedThread.h"
#include "JsonConfig.h"

using namespace std;

namespace db_adapter {

class DatabaseMappingOrchestrator : public ThreadMethod {
   public:
    DatabaseMappingOrchestrator(shared_ptr<DatabaseMappingStrategy> strategy);
    virtual ~DatabaseMappingOrchestrator();

    bool thread_one_step() override;
    bool is_finished() const;

   private:
    shared_ptr<DatabaseMappingStrategy> strategy;

    vector<MappingTask> tasks;
    size_t current_task = 0;
    bool finished = false;
    bool initialized = false;
};

}  // namespace db_adapter
