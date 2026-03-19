#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "AtomDBSingleton.h"
#include "DedicatedThread.h"
#include "PostgresWrapper.h"
#include "Processor.h"
#include "SharedQueue.h"

#define BATCH_SIZE 5000

using namespace atomdb;
using namespace std;
using namespace commons;
using namespace atoms;
using namespace db_adapter;

namespace db_adapter {

class DatabaseMappingJob : public ThreadMethod {
   public:
    DatabaseMappingJob(const string& host,
                       int port,
                       const string& database,
                       const string& user,
                       const string& password,
                       MAPPER_TYPE mapper_type = MAPPER_TYPE::SQL2ATOMS,
                       shared_ptr<SharedQueue> output_queue = nullptr);
    ~DatabaseMappingJob();

    void add_task_query(const string& virtual_name, const string& query);
    void add_task_table(TableMapping table_mapping);

    bool thread_one_step() override;

    bool is_finished() const;

   protected:
    struct MappingTask {
        enum Type { TABLE, QUERY } type;
        TableMapping table_mapping;
        string virtual_name;
        string query;
    };
    unique_ptr<PostgresDatabaseConnection> db_conn;
    unique_ptr<PostgresWrapper> wrapper;
    vector<MappingTask> tasks;
    size_t current_task = 0;
    bool finished = false;
    bool initialized = false;
};

class AtomPersistenceJob : public ThreadMethod {
   public:
    AtomPersistenceJob(shared_ptr<SharedQueue> input_queue);
    ~AtomPersistenceJob();

    bool thread_one_step() override;
    bool is_finished() const;
    void set_producer_finished();

   protected:
    atomic<int> count = 0;
    vector<Atom*> atoms;
    shared_ptr<SharedQueue> input_queue;
    shared_ptr<AtomDB> atomdb;
    bool finished = false;
    bool producer_finished = false;
};

class AtomPersistenceJob2 {
   public:
    AtomPersistenceJob2(shared_ptr<SharedQueue> input_queue);
    ~AtomPersistenceJob2();

    void consumer_task();
    void set_producer_finished();
    shared_ptr<AtomDB> get_atomdb() const { return atomdb; }

   protected:
    atomic<int> total_count{0};
    shared_ptr<SharedQueue> input_queue;
    shared_ptr<AtomDB> atomdb;
    atomic<bool> producer_finished{false};
};

}  // namespace db_adapter
