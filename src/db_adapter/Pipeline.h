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

class PostgresWrapperJob : public ThreadMethod {
   public:
    PostgresWrapperJob(shared_ptr<PostgresWrapper> db);
    ~PostgresWrapperJob() = default;

    void add_task_query(const string& virtual_name, const string& query);
    void add_task_table(TableMapping table_mapping);

    bool thread_one_step() override;

   protected:
    struct MappingTask {
        enum Type { TABLE, QUERY } type;
        TableMapping table_mapping;
        string virtual_name;
        string query;
    };
    shared_ptr<PostgresWrapper> db;
    vector<MappingTask> tasks;
    size_t current_task = 0;
};

class AtomDBJob : public ThreadMethod {
   public:
    AtomDBJob(shared_ptr<SharedQueue> input_queue);
    ~AtomDBJob() = default;

    bool thread_one_step() override;

   protected:
    shared_ptr<SharedQueue> input_queue;
    shared_ptr<AtomDB> atomdb;
};


class Producer : public DedicatedThread {
    public:
     Producer(const string& id, ThreadMethod* job, shared_ptr<PostgresWrapper> wrapper);
     ~Producer() = default;

     void setup();
     void start();
     void stop();

    protected:
     shared_ptr<PostgresWrapper> wrapper;
}

// =============================================================================
// Producer
// =============================================================================
// class ProducerProcessor : public Processor {
//    public:
//     ProducerProcessor(const string& id, shared_ptr<SharedQueue> output_queue);
//     virtual ~ProducerProcessor() = default;

//     void setup() override;
//     void start() override;
//     void stop() override;

//    protected:
//     virtual void on_setup() = 0;
//     virtual void on_start() = 0;
//     virtual void on_stop() = 0;
//     virtual bool produce_one() = 0;

//     shared_ptr<SharedQueue> output_queue;

//    private:
//     class ProducerThreadMethod : public ThreadMethod {
//        public:
//         ProducerThreadMethod(ProducerProcessor* owner) : owner(owner) {}
//         bool thread_one_step() override { return owner->produce_one(); }

//        private:
//         ProducerProcessor* owner;
//     };

//     unique_ptr<ProducerThreadMethod> thread_method;
//     shared_ptr<DedicatedThread> thread;
// };

// // =============================================================================
// // Consumer
// // =============================================================================
// class ConsumerProcessor : public Processor {
//    public:
//     ConsumerProcessor(const string& id, shared_ptr<SharedQueue> input_queue);
//     virtual ~ConsumerProcessor() = default;

//     void setup() override;
//     void start() override;
//     void stop() override;

//    protected:
//     virtual void on_setup() = 0;
//     virtual void on_start() = 0;
//     virtual void on_stop() = 0;
//     virtual bool process_one() = 0;

//     shared_ptr<SharedQueue> input_queue;

//    private:
//     class ConsumerThreadMethod : public ThreadMethod {
//        public:
//         ConsumerThreadMethod(ConsumerProcessor* owner) : owner(owner) {}
//         bool thread_one_step() override { return owner->process_one(); }

//        private:
//         ConsumerProcessor* owner;
//     };

//     unique_ptr<ConsumerThreadMethod> thread_method;
//     shared_ptr<DedicatedThread> thread;
// };

// class PipelineProcessor : public Processor {
//    public:
//     PipelineProcessor(const string& id, shared_ptr<SharedQueue> queue);
//     virtual ~PipelineProcessor() = default;

//     void setup() override;
//     void start() override;
//     void stop() override;

//    protected:
//     virtual void on_setup() = 0;
//     virtual void on_start() = 0;
//     virtual void on_stop() = 0;
//     virtual bool process_one() = 0;

//     shared_ptr<SharedQueue> queue;

//    private:
//     class PipelineThreadMethod : public ThreadMethod {
//        public:
//         PipelineThreadMethod(PipelineProcessor* owner) : owner(owner) {}
//         bool thread_one_step() override { return owner->process_one(); }

//        private:
//         PipelineProcessor* owner;
//     };

//     unique_ptr<PipelineThreadMethod> thread_method;
//     shared_ptr<DedicatedThread> thread;
// };

// class PostgresProducerProcessor : public PipelineProcessor {
//    public:
//     PostgresProducerProcessor(const string& id,
//                               shared_ptr<PostgresWrapper> db,
//                               shared_ptr<SharedQueue> output_queue);
//     ~PostgresProducerProcessor() = default;

//     void add_task_query(const string& virtual_name, const string& query);
//     void add_task_table(TableMapping table_mapping);

//    protected:
//     void on_setup() override;
//     void on_start() override;
//     bool produce_one() override;
//     void on_stop() override;

//    private:
//     shared_ptr<PostgresWrapper> db;
//     vector<MappingTask> tasks;
//     size_t current_task = 0;
// };

// class AtomDBConsumerProcessor : public ConsumerProcessor {
//    public:
//     AtomDBConsumerProcessor(const string& id,
//                             shared_ptr<SharedQueue> input_queue,
//                             const string& das_host,
//                             int das_port);
//     ~AtomDBConsumerProcessor() = default;

//    protected:
//     void on_setup() override;
//     void on_start() override;
//     bool process_one() override;
//     void on_stop() override;

//    private:
//     shared_ptr<AtomDB> atomdb;
//     string das_host;
//     int das_port;
// };

}  // namespace db_adapter
