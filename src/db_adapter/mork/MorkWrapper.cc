#include "MorkWrapper.h"

#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace atoms;

// ==============================
//  Construction / destruction
// ==============================

MorkConnection::MorkConnection(const string& id, const string& host, int port)
    : DatabaseConnection(id, host, port) {
    RAISE_ERROR("MorkConnection constructor not implemented yet");
}

MorkConnection::~MorkConnection() {}

// ==============================
// Public
// ==============================

void MorkConnection::connect() {
    RAISE_ERROR("MorkConnection::connect() not implemented yet");
    return;
}

void MorkConnection::disconnect() {
    RAISE_ERROR("MorkConnection::disconnect() not implemented yet");
    return;
}

vector<string> MorkConnection::query(const string& metta_query) {
    RAISE_ERROR("MorkConnection::query() not implemented yet");
    return {};
}

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
