#ifndef LOG_LEVEL
#define LOG_LEVEL INFO_LEVEL
#endif
#include "DatabaseConnection.h"

#include "Logger.h"
using namespace db_adapter;

DatabaseConnection::DatabaseConnection(const string& id, const string& host, int port) : Processor(id) {
    this->host = host;
    this->port = port;
    this->connected = false;
}

DatabaseConnection::~DatabaseConnection() {}

void DatabaseConnection::start() {
    if (this->is_running() || this->is_finished()) return;

    {
        lock_guard<mutex> lock(this->connection_mutex);
        this->connect();
        this->connected = true;
    }

    Processor::start();
}

void DatabaseConnection::stop() {
    if (!this->is_running()) return;

    {
        lock_guard<mutex> lock(this->connection_mutex);
        this->disconnect();
        this->connected = false;
    }

    Processor::stop();
}

bool DatabaseConnection::is_connected() const { return this->connected; }
