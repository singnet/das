#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "AtomDB.h"
#include "DataMapper.h"
#include "PostgresWrapper.h"
#include "SharedQueue.h"

using namespace std;
using namespace commons;
using namespace atomdb;

namespace db_adapter {

class Orchestrator {
   public:
    Orchestrator(DBWrapper& wrapper, AtomDB& db, SharedQueue& queue);
    ~Orchestrator();

    void start();
    void stop();

   private:
    void worker_loop();

    shared_ptr<DBWrapper> wrapper;
    shared_ptr<AtomDB> db;
    SharedQueue queue;
    atomic<bool> running{false};
    thread worker;
};

};  // namespace db_adapter