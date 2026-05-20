#include "MorkMappingStrategy.h"

using namespace std;
using namespace db_adapter;

// ==============================
//  Construction / destruction
// ==============================

db_adapter::MorkMappingStrategy::MorkMappingStrategy(const JsonConfig& config,
                                                     shared_ptr<MorkConnection> conn,
                                                     shared_ptr<BoundedSharedQueue> queue)
    : DatabaseMappingStrategy(conn), config(config) {
    this->wrapper = make_shared<MorkWrapper>(*conn, queue);
}

// ==============================
//  Public
// ============================

vector<MappingTask> MorkMappingStrategy::create_tasks() {
    RAISE_ERROR("MorkMappingStrategy::create_tasks() not implemented yet");
    return vector<MappingTask>{};
}

void MorkMappingStrategy::execute_task(const MappingTask& task) {
    RAISE_ERROR("MorkMappingStrategy::execute_task() not implemented yet");
    return;
}
