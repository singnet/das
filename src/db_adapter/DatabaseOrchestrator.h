#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "AtomDB.h"
#include "BoundedSharedQueue.h"
#include "DedicatedThread.h"
#include "JsonConfig.h"
#include "MorkWrapper.h"
#include "PostgresWrapper.h"

using namespace atomdb;
using namespace std;
using namespace commons;
using namespace atoms;
using namespace db_adapter;

namespace db_adapter {

struct MappingTask {
    string task_name;
    optional<string> context;
};

class DatabaseMappingStrategy {
   public:
    virtual ~DatabaseMappingStrategy() = default;
    virtual vector<MappingTask> create_tasks(const JsonConfig& config) = 0;
    virtual void execute_task(const MappingTask& task) = 0;
};

class DatabaseMappingOrchestrator : public ThreadMethod {
   public:
    DatabaseMappingOrchestrator(const JsonConfig& config, shared_ptr<BoundedSharedQueue> output_queue);
    virtual ~DatabaseMappingOrchestrator();

    bool thread_one_step() override;
    bool is_finished() const;

   private:
    void database_setup(const JsonConfig& config, shared_ptr<BoundedSharedQueue> output_queue);

    unique_ptr<DatabaseConnection> db_conn;
    unique_ptr<DatabaseMappingStrategy> strategy;

    vector<MappingTask> tasks;
    size_t current_task = 0;
    bool finished = false;
    bool initialized = false;
};

class PostgresMappingStrategy : public DatabaseMappingStrategy {
   public:
    explicit PostgresMappingStrategy(unique_ptr<PostgresWrapper> wrapper);
    vector<MappingTask> create_tasks(const JsonConfig& config) override;
    void execute_task(const MappingTask& task) override;

   private:
    unique_ptr<PostgresWrapper> wrapper;
};

class MorkMappingStrategy : public DatabaseMappingStrategy {
   public:
    explicit MorkMappingStrategy(unique_ptr<MorkWrapper> wrapper);
    vector<MappingTask> create_tasks(const JsonConfig& config) override;
    void execute_task(const MappingTask& task) override;

   private:
    unique_ptr<MorkWrapper> wrapper;
};

}  // namespace db_adapter
