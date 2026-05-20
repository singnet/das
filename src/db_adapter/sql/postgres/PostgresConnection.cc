#include "PostgresConnection.h"

#include "Utils.h"

using namespace std;
using namespace commons;
using namespace db_adapter;

// ==============================
//  Construction / destruction
// ==============================

PostgresConnection::PostgresConnection(const string& id,
                                       const string& host,
                                       int port,
                                       const string& database,
                                       const string& user,
                                       const string& password)
    : DatabaseConnection(id, host, port), database(database), user(user), password(password) {}

PostgresConnection::~PostgresConnection() {
    this->close_cursor();
    this->disconnect();
}

// ==============================
//  Public
// ==============================

void PostgresConnection::connect() {
    LOG_INFO("Connecting to PostgreSQL database at " << host << ":" << port << "...");
    try {
        string conn_str = "host=" + host + " port=" + std::to_string(port) + " dbname=" + database;
        if (!user.empty()) {
            conn_str += " user=" + user;
        }
        if (!password.empty()) {
            conn_str += " password=" + password;
        }
        LOG_DEBUG("Connection string: " << conn_str);
        this->conn = make_unique<pqxx::connection>(conn_str);
    } catch (const exception& e) {
        RAISE_ERROR("Could not connect to database: " + string(e.what()));
    }
}

void PostgresConnection::disconnect() {
    if (this->conn) {
        this->conn->close();
        this->conn.reset();
    }
}

pqxx::result PostgresConnection::execute_query(const string& query) {
    if (!this->conn || !this->conn->is_open()) {
        RAISE_ERROR("Postgres connection is not open.");
    }

    try {
        pqxx::work transaction(*this->conn);
        pqxx::result result = transaction.exec(query);
        transaction.commit();
        return result;
    } catch (const exception& e) {
        RAISE_ERROR("Error during query execution: " + string(e.what()));
    }
    return pqxx::result{};
}

void PostgresConnection::begin_cursor(const string& cursor_name, const string& query) {
    if (!this->conn || !this->conn->is_open()) {
        RAISE_ERROR("Postgres connection is not open.");
    }
    if (this->transaction) {
        RAISE_ERROR("A transaction is already active. Close the current cursor first.");
    }
    this->transaction = make_unique<pqxx::work>(*this->conn);

    try {
        this->transaction->exec("DECLARE " + cursor_name + " CURSOR FOR " + query);
    } catch (const exception& e) {
        this->transaction.reset();
        RAISE_ERROR("Error executing cursor query: " + string(e.what()));
    }
}

pqxx::result PostgresConnection::fetch_cursor(const string& cursor_name, size_t limit) {
    if (!this->transaction) {
        RAISE_ERROR("No active transaction. Call begin_cursor first.");
    }
    return this->transaction->exec("FETCH " + std::to_string(limit) + " FROM " + cursor_name);
}

void PostgresConnection::close_cursor() {
    if (this->transaction) {
        try {
            this->transaction->commit();
        } catch (...) {
            // Ignore errors during commit on close
        }
        this->transaction.reset();
    }
}