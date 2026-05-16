#include "MorkWrapper.h"

#include "MorkConnection.h"
#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace db_adapter;

// ==============================
//  Construction / destruction
// ==============================

MorkWrapper::MorkWrapper(MorkConnection& conn,
                         shared_ptr<BoundedSharedQueue> output_queue,
                         MAPPER_TYPE mapper_type)
    : DatabaseWrapper(conn, mapper_type), conn(conn), output_queue(output_queue) {
    RAISE_ERROR("MorkWrapper constructor not implemented yet");
}

MorkWrapper::~MorkWrapper() {}

void MorkWrapper::map(const string& metta_query) {
    RAISE_ERROR("MorkWrapper::map() not implemented yet");
    return;
}
