#pragma once

#include <memory>
#include <pqxx/pqxx>
#include <string>
#include <unordered_set>

#include "DatabaseConnection.h"

using namespace std;

namespace db_adapter {

inline const unordered_set<pqxx::oid> TIME_TYPE_OIDS = {
    1082,  // date
    1083,  // time without time zone
    1114,  // timestamp without time zone
    1184,  // timestamp with time zone
    1186,  // interval
    1266,  // time with time zone
};

inline bool is_time_type(pqxx::oid oid) { return TIME_TYPE_OIDS.count(oid) > 0; }

class PostgresConnection : public DatabaseConnection {
   public:
    PostgresConnection(const string& id,
                       const string& host,
                       int port,
                       const string& database,
                       const string& user,
                       const string& password);
    ~PostgresConnection();

    void connect() override;
    void disconnect() override;

    pqxx::result execute_query(const string& query);

    void begin_cursor(const string& cursor_name, const string& query);
    pqxx::result fetch_cursor(const string& cursor_name, size_t limit);
    void close_cursor();

   protected:
    unique_ptr<pqxx::connection> conn;
    unique_ptr<pqxx::work> transaction;
    string database;
    string user;
    string password;
};

}  // namespace db_adapter