#pragma once
#include <memory>
#include <pqxx/pqxx>
#include <string>

using namespace std;

/**
 * @class PostgresWrapper
 * @brief A class that provides a high-level interface to interact with a PostgreSQL database.
 *
 * The PostgresWrapper is responsible for managing the connection to a PostgreSQL database,
 * building and executing SQL queries. It encapsulates the connection parameters and provides
 * a simple interface for database operations.
 */
class PostgresWrapper {
   public:
    /**
     * @brief Constructs a PostgresWrapper with the specified connection parameters.
     *
     * @param host The hostname or IP address of the PostgreSQL server.
     * @param port The port number of the PostgreSQL server.
     * @param database The name of the database to connect to (default: "postgres").
     * @param user The username for authentication.
     * @param password The password for authentication.
     */
    PostgresWrapper(const string& host,
                    int port,
                    const string& database = "postgres",
                    const string& user = "postgres",
                    const string& password = "postgres");
    ~PostgresWrapper();

    /**
     * @brief Establishes a connection to the PostgreSQL database.
     *
     * @return A unique pointer to a pqxx::connection object.
     * @throws runtime_error if the connection fails.
     */
    unique_ptr<pqxx::connection> connect();

    void execute_query(const string& query);

   private:
    string host_;
    int port_;
    string database_;
    string user_;
    string password_;

    /**
     * @brief Builds the connection string from the stored parameters.
     *
     * @return A PostgreSQL connection string.
     */
    string build_connection_string() const;
};
