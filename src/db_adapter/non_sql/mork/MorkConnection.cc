#include "MorkConnection.h"

#include "Utils.h"

using namespace std;
using namespace atomdb;
using namespace db_adapter;

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