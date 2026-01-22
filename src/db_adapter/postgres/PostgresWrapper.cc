#include "PostgresWrapper.h"

#include <memory>
#include <pqxx/pqxx>
#include <stdexcept>
#include <string>

using namespace std;

PostgresWrapper::PostgresWrapper(
    const string& host, int port, const string& database, const string& user, const string& password)
    : host_(host), port_(port), database_(database), user_(user), password_(password) {}

PostgresWrapper::~PostgresWrapper() {}

unique_ptr<pqxx::connection> PostgresWrapper::connect() {
    try {
        string connection_string = build_connection_string();
        return make_unique<pqxx::connection>(connection_string);
    } catch (const pqxx::sql_error& e) {
        throw runtime_error("Could not connect to database: " + string(e.what()));
    } catch (const exception& e) {
        throw runtime_error("Could not connect to database: " + string(e.what()));
    }
}

string PostgresWrapper::build_connection_string() const {
    string conn_str = "host=" + host_ + " port=" + to_string(port_) + " dbname=" + database_;

    if (!user_.empty()) {
        conn_str += " user=" + user_;
    }
    if (!password_.empty()) {
        conn_str += " password=" + password_;
    }

    return conn_str;
}
