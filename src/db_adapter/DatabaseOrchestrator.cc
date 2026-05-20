#include "DatabaseOrchestrator.h"

#include <string>

#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace commons;
using namespace db_adapter;

// ==============================
//  Construction / destruction
// ==============================

DatabaseMappingOrchestrator::DatabaseMappingOrchestrator(shared_ptr<DatabaseMappingStrategy> strategy)
    : strategy(strategy) {
    this->tasks = this->strategy->create_tasks();
}

DatabaseMappingOrchestrator::~DatabaseMappingOrchestrator() {
    if (this->is_finished()) {
        this->strategy->stop_connection();
    }
}

// ==============================
//  Public
// ==============================

bool DatabaseMappingOrchestrator::thread_one_step() {
    LOG_DEBUG("DatabaseMappingOrchestrator thread_one_step called. Current task index: "
              << this->current_task);
    if (this->current_task >= this->tasks.size()) {
        this->strategy->stop_connection();
        return false;
    }

    if (!this->initialized) {
        this->strategy->start_connection();
        this->initialized = true;
    }

    auto& task = this->tasks[this->current_task];

    LOG_DEBUG("Processing task " << task.task_name);

    this->strategy->execute_task(task);

    this->current_task++;
    this->finished = (this->current_task >= this->tasks.size());
    return !this->finished;
}

bool DatabaseMappingOrchestrator::is_finished() const { return this->finished; }
