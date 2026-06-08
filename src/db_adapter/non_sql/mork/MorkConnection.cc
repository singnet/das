#include "MorkConnection.h"

#include "Utils.h"

using namespace std;
using namespace atomdb;
using namespace db_adapter;

// ==============================
//  Construction / destruction
// ==============================

MorkConnection::MorkConnection(const string& id, const string& host, int port)
    : DatabaseConnection(id, host, port) {}

MorkConnection::~MorkConnection() {}

// ==============================
// Public
// ==============================

void MorkConnection::connect() {
    string address = this->host + ":" + std::to_string(this->port);
    try {
        this->conn = make_shared<MorkClient>(address);
        LOG_DEBUG("Connected to MORK at " << address);
    } catch (const exception& e) {
        RAISE_ERROR("Could not connect to MORK at " + address + ": " + string(e.what()));
    }
}

void MorkConnection::disconnect() {
    LOG_DEBUG("MORK connection does not require disconnection");
    return;
}

vector<string> MorkConnection::query(const string& metta_query) {
    return this->conn->get(metta_query, metta_query);
}