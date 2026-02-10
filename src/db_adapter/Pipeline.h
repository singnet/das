#include "AtomDBSingleton.h"
#include "DedicatedThread.h"
#include "PostgresWrapper.h"
#include "Processor.h"
#include "SharedQueue.h"

#define BATCH_SIZE 1000

using namespace atomdb;
using namespace std;
using namespace commons;
using namespace atoms;
using namespace db_adapter;

namespace db_adapter {

struct MappingTask {
    enum Type { TABLE, QUERY } type;
    TableMapping table_mapping;
    string virtual_name;
    string query;
};

class PostgresWrapperJob : public ThreadMethod {
   public:
    PostgresWrapperJob(shared_ptr<PostgresWrapper> db);
    ~PostgresWrapperJob() = default;

    void add_task_query(const string& virtual_name, const string& query);
    void add_task_table(TableMapping table_mapping);

    bool thread_one_step() override;

   protected:
    shared_ptr<PostgresWrapper> db;
    vector<MappingTask> tasks;
    size_t current_task = 0;
};

class WorkerJob : public ThreadMethod {
   public:
    WorkerJob(shared_ptr<SharedQueue> input_queue);
    ~WorkerJob() = default;

    bool thread_one_step() override;

   protected:
    shared_ptr<SharedQueue> input_queue;
    shared_ptr<AtomDB> atomdb;
};

}  // namespace db_adapter

   // // resources
// auto queue = make_shared<SharedQueue>();
// auto wrapper = make_shared<PostgresWrapper>(
//     "localhost", 5433, "postgres_wrapper_test", "postgres", "test", MAPPER_TYPE::SQL2ATOMS, queue);

// // Jobs
// PostgresWrapperJob producer_job(wrapper);
// WorkerJob worker_job(queue);

// // Threads
// auto producer = make_shared<DedicatedThread>("psql", &producer_job);
// auto worker = make_shared<DedicatedThread>("worker", &worker_job);

// auto pipeline = make_shared<Processor>("pipeline");
// Processor::bind_subprocessor(pipeline, producer);
// Processor::bind_subprocessor(pipeline, worker);

// pipeline->setup();
// pipeline->start();
// pipeline->stop();