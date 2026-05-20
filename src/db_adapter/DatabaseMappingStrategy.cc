#include "DatabaseMappingStrategy.h"

using namespace db_adapter;

DatabaseMappingStrategy::DatabaseMappingStrategy(shared_ptr<DatabaseConnection> conn) : conn(conn) {}

void DatabaseMappingStrategy::start_connection() {
    this->conn->setup();
    this->conn->start();
}
void DatabaseMappingStrategy::stop_connection() { this->conn->stop(); }
