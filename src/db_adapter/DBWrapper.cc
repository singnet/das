#include "DBWrapper.h"

using namespace db_adapter;

DBConnection::DBConnection(const string& id, const string& host, int port) : Processor(id) {
    this->host = host;
    this->port = port;
    this->connected = false;
    this->setup();
}

DBConnection::~DBConnection() {}

void DBConnection::setup() {
    if (!this->is_setup()) {
        Processor::setup();
    }
}

void DBConnection::start() {
    if (this->is_running() || this->is_finished()) return;

    {
        lock_guard<mutex> lock(this->connection_mutex);
        this->connect();
        this->connected = true;
    }

    Processor::start();
}

void DBConnection::stop() {
    if (!this->is_running()) return;

    {
        lock_guard<mutex> lock(this->connection_mutex);
        this->disconnect();
        this->connected = false;
    }

    Processor::stop();
}

bool DBConnection::is_connected() const { return this->connected; }
