#include "Orchestrator.h"

#include "AtomDB.h"
#include "DBWrapper.h"
#include "SharedQueue.h"

using namespace db_adapter;
using namespace std;
using namespace commons;
using namespace atomdb;

db_adapter::Orchestrator::Orchestrator(DBWrapper& wrapper, AtomDB& db, SharedQueue& queue) {}

Orchestrator::~Orchestrator() {}

void Orchestrator::start() {}

void Orchestrator::stop() {}
