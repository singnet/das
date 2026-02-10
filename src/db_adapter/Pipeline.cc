#include "DedicatedThread.h"
#include "PostgresWrapper.h"
#include "Processor.h"
#include "SharedQueue.h"

class PostgresWrapperJob : public ThreadMethod {
   public:
    PostgresWrapperJob(const string& host,
                       int port,
                       const string& database,
                       const string& user,
                       const string& password,
                       MAPPER_TYPE mapper_type = MAPPER_TYPE::SQL2ATOMS) {
        this->db = make_shared<PostgresWrapper>(host, port, database, user, password, mapper_type);
    }
    ~PostgresWrapperJob() = default;

    bool thread_one_step() override {
        try {
            this->db->map_table();
        } catch (const exception& e) {
            Utils::error("Error in PostgresWrapperProducer: " + string(e.what()));
        }
        return false;
    }

    void set_output_queue(shared_ptr<SharedQueue> output_queue) { this->output_queue = output_queue; }

   protected:
    shared_ptr<PostgresWrapper> db;
    shared_ptr<SharedQueue> output_queue;
};

class WorkerJob : public ThreadMethod {
   public:
    bool thread_one_step() override {
        try {
            auto request = this->input_queue->dequeue();
            if (request) {
                // Process the request
            }
        } catch (const exception& e) {
            Utils::error("Error in Worker: " + string(e.what()));
        }
        return false;
    }

    void set_input_queue(shared_ptr<SharedQueue> input_queue) { this->input_queue = input_queue; }

   protected:
    shared_ptr<SharedQueue> input_queue;
};

// resources
ProstgresWrapperJob producer_job("localhost", 5433, "postgres_wrapper_test", "postgres", "test");
WorkerJob worker_job;

auto queue = make_shared<SharedQueue>();
auto db = make_shared<PostgresWrapper>("localhost", 5433, "postgres_wrapper_test", "postgres", "test");

// Jobs
