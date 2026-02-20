#include <gtest/gtest.h>

#include <atomic>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "Atom.h"
#include "ContextLoader.h"
#include "DataTypes.h"
#include "DedicatedThread.h"
#include "Logger.h"
#include "Node.h"
#include "Pipeline.h"
#include "PostgresWrapper.h"
#include "Processor.h"
#include "TestConfig.h"

using namespace std;
using namespace db_adapter;
using namespace atoms;
using namespace processor;

class PostgresWrapperTestEnvironment : public ::testing::Environment {
   public:
    void SetUp() override {
        TestConfig::load_environment();
        AtomDBSingleton::init();
    }

    void TearDown() override {}
};

class PostgresDatabaseConnectionTest : public ::testing::Test {
   protected:
    string TEST_HOST = "localhost";
    int TEST_PORT = 5433;
    string TEST_DB = "postgres_wrapper_test";
    string TEST_USER = "postgres";
    string TEST_PASSWORD = "test";

    string INVALID_HOST = "invalid.host";
    int INVALID_PORT = 99999;
    string INVALID_DB = "database_xyz";

    string FEATURE_TABLE = "public.feature";
    string ORGANISM_TABLE = "public.organism";
    string CVTERM_TABLE = "public.cvterm";
    string FEATURE_PK = "feature_id";
    string ORGANISM_PK = "organism_id";
    string CVTERM_PK = "cvterm_id";

    int DROSOPHILA_ORGANISM_ID = 1;
    int HUMAN_ORGANISM_ID = 2;

    int WHITE_GENE_ID = 1;
    string WHITE_GENE_NAME = "white";
    string WHITE_GENE_UNIQUENAME = "FBgn0003996";

    int TOTAL_ROWS_ORGANISMS = 5;
    int TOTAL_ROWS_CVTERMS = 10;
    int TOTAL_ROWS_FEATURES = 26;

    void SetUp() override {}

    void TearDown() override {}

    shared_ptr<PostgresDatabaseConnection> create_db_connection() {
        auto conn = make_shared<PostgresDatabaseConnection>(
            "test-conn", TEST_HOST, TEST_PORT, TEST_DB, TEST_USER, TEST_PASSWORD);
        conn->setup();
        conn->start();
        return conn;
    }
};

class PostgresWrapperTest : public ::testing::Test {
   protected:
    string TEST_HOST = "localhost";
    int TEST_PORT = 5433;
    string TEST_DB = "postgres_wrapper_test";
    string TEST_USER = "postgres";
    string TEST_PASSWORD = "test";

    string INVALID_HOST = "invalid.host";
    int INVALID_PORT = 99999;
    string INVALID_DB = "database_xyz";

    string FEATURE_TABLE = "public.feature";
    string ORGANISM_TABLE = "public.organism";
    string CVTERM_TABLE = "public.cvterm";
    string FEATURE_PK = "feature_id";
    string ORGANISM_PK = "organism_id";
    string CVTERM_PK = "cvterm_id";

    int DROSOPHILA_ORGANISM_ID = 1;
    int HUMAN_ORGANISM_ID = 2;

    int WHITE_GENE_ID = 1;
    string WHITE_GENE_NAME = "white";
    string WHITE_GENE_UNIQUENAME = "FBgn0003996";

    int TOTAL_ROWS_ORGANISMS = 5;
    int TOTAL_ROWS_CVTERMS = 10;
    int TOTAL_ROWS_FEATURES = 26;

    void SetUp() override {
        temp_file_path = "/tmp/context.json";

        ofstream file(temp_file_path);
        file << R"([
            {
                "type": 1,
                "tables": [
                {
                    "table_name": "public.organism",
                    "skip_columns": [],
                    "where_clauses": ["organism_id = 1"]
                },
                {
                    "table_name": "public.feature",
                    "skip_columns": [],
                    "where_clauses": ["feature_id = 1"]
                },
                {
                    "table_name": "public.cvterm",
                    "skip_columns": [],
                    "where_clauses": ["cvterm_id = 1"]
                }
                ]
            }
            ])";
        file.close();
    }

    void TearDown() override { std::remove(temp_file_path.c_str()); }

    shared_ptr<PostgresWrapper> create_wrapper(PostgresDatabaseConnection& db_conn,
                                               MAPPER_TYPE mapper_type = MAPPER_TYPE::SQL2ATOMS) {
        auto queue = make_shared<SharedQueue>();
        return make_shared<PostgresWrapper>(db_conn, mapper_type, queue);
    }

    string temp_file_path;

    shared_ptr<PostgresDatabaseConnection> create_db_connection() {
        auto conn = make_unique<PostgresDatabaseConnection>(
            "test-conn", TEST_HOST, TEST_PORT, TEST_DB, TEST_USER, TEST_PASSWORD);
        conn->setup();
        conn->start();
        return conn;
    }
};

TEST_F(PostgresDatabaseConnectionTest, Connection) {
    auto conn = create_db_connection();

    EXPECT_TRUE(conn->is_connected());

    auto result = conn->execute_query("SELECT 1");

    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result[0][0].as<int>(), 1);

    conn->stop();

    EXPECT_FALSE(conn->is_connected());

    auto conn1 = new PostgresDatabaseConnection(
        "test-conn1", INVALID_HOST, TEST_PORT, TEST_DB, TEST_USER, TEST_PASSWORD);
    EXPECT_THROW(conn1->connect(), std::runtime_error);

    auto conn2 = new PostgresDatabaseConnection(
        "test-conn2", TEST_HOST, INVALID_PORT, TEST_DB, TEST_USER, TEST_PASSWORD);
    EXPECT_THROW(conn2->connect(), std::runtime_error);

    auto conn3 = new PostgresDatabaseConnection(
        "test-conn3", TEST_HOST, TEST_PORT, INVALID_DB, TEST_USER, TEST_PASSWORD);
    EXPECT_THROW(conn3->connect(), std::runtime_error);
}

TEST_F(PostgresDatabaseConnectionTest, ConcurrentConnection) {
    const int num_threads = 100;
    vector<thread> threads;
    atomic<int> count_threads{0};

    auto worker = [&](int thread_id) {
        try {
            string thread_id_str = "conn-" + to_string(thread_id);
            auto conn = new PostgresDatabaseConnection(
                thread_id_str, TEST_HOST, TEST_PORT, TEST_DB, TEST_USER, TEST_PASSWORD);

            EXPECT_FALSE(conn->is_connected());

            conn->setup();

            EXPECT_FALSE(conn->is_connected());

            conn->start();

            EXPECT_TRUE(conn->is_connected());

            conn->execute_query("SELECT 1");

            count_threads++;

            conn->stop();

            EXPECT_FALSE(conn->is_connected());
        } catch (const exception& e) {
            cout << "Thread " << thread_id << " failed with error: " << e.what() << endl;
        }
    };

    for (int i = 0; i < num_threads; ++i) threads.emplace_back(worker, i);

    for (auto& t : threads) t.join();

    EXPECT_EQ(count_threads, num_threads);
}

TEST_F(PostgresDatabaseConnectionTest, CheckData) {
    auto conn = create_db_connection();

    auto result = conn->execute_query(
        "SELECT organism_id, genus, species, common_name FROM organism WHERE organism_id = 1");

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0]["organism_id"].as<int>(), 1);
    EXPECT_EQ(result[0]["genus"].as<string>(), "Drosophila");
    EXPECT_EQ(result[0]["species"].as<string>(), "melanogaster");
    EXPECT_EQ(result[0]["common_name"].as<string>(), "fruit fly");

    auto result2 =
        conn->execute_query("SELECT feature_id, name, uniquename FROM feature WHERE feature_id = " +
                            to_string(WHITE_GENE_ID));

    ASSERT_EQ(result2.size(), 1);
    EXPECT_EQ(result2[0]["feature_id"].as<int>(), WHITE_GENE_ID);
    EXPECT_EQ(result2[0]["name"].as<string>(), WHITE_GENE_NAME);
    EXPECT_EQ(result2[0]["uniquename"].as<string>(), WHITE_GENE_UNIQUENAME);

    auto result3 = conn->execute_query("SELECT COUNT(*) FROM organism");

    EXPECT_EQ(result3[0][0].as<int>(), TOTAL_ROWS_ORGANISMS);

    auto result4 = conn->execute_query("SELECT COUNT(*) FROM cvterm");

    EXPECT_EQ(result4[0][0].as<int>(), TOTAL_ROWS_CVTERMS);

    auto result5 = conn->execute_query("SELECT COUNT(*) FROM feature");

    EXPECT_EQ(result5[0][0].as<int>(), TOTAL_ROWS_FEATURES);
}

TEST_F(PostgresWrapperTest, GetTable) {
    auto conn = create_db_connection();
    auto wrapper = create_wrapper(*conn);
    auto tables = wrapper->list_tables();
    ASSERT_FALSE(tables.empty());

    string target_name = tables[0].name;
    Table t = wrapper->get_table(target_name);

    EXPECT_EQ(t.name, target_name);
    EXPECT_EQ(t.primary_key, tables[0].primary_key);

    Table feature = wrapper->get_table("public.feature");

    EXPECT_EQ(feature.name, "public.feature");
    EXPECT_EQ(feature.primary_key, "feature_id");

    EXPECT_THROW(wrapper->get_table("fake_table_name"), std::runtime_error);
}

TEST_F(PostgresWrapperTest, ListTables) {
    auto conn = create_db_connection();
    auto wrapper = create_wrapper(*conn);

    auto tables = wrapper->list_tables();

    EXPECT_GE(tables.size(), 3);

    bool found_organism = false;
    bool found_cvterm = false;
    bool found_feature = false;

    for (const auto& table : tables) {
        if (table.name == ORGANISM_TABLE) found_organism = true;
        if (table.name == CVTERM_TABLE) found_cvterm = true;
        if (table.name == FEATURE_TABLE) found_feature = true;
    }

    EXPECT_TRUE(found_organism);
    EXPECT_TRUE(found_cvterm);
    EXPECT_TRUE(found_feature);

    vector<Table> tables_cached = wrapper->list_tables();

    ASSERT_EQ(tables_cached.size(), tables_cached.size());
    if (!tables_cached.empty()) {
        EXPECT_EQ(tables_cached[0].name, tables_cached[0].name);
    }
}

TEST_F(PostgresWrapperTest, TablesStructure) {
    auto conn = create_db_connection();
    auto wrapper = create_wrapper(*conn);

    Table organism_table = wrapper->get_table(ORGANISM_TABLE);

    EXPECT_EQ(organism_table.name, ORGANISM_TABLE);
    EXPECT_EQ(organism_table.primary_key, ORGANISM_PK);
    EXPECT_TRUE(organism_table.foreign_keys.empty());

    vector<string> expected_cols = {
        "organism_id", "genus", "species", "common_name", "abbreviation", "comment", "created_at"};
    for (const auto& expected : expected_cols) {
        bool found =
            find(organism_table.column_names.begin(), organism_table.column_names.end(), expected) !=
            organism_table.column_names.end();
        EXPECT_TRUE(found);
    }

    Table cvterm_table = wrapper->get_table(CVTERM_TABLE);

    EXPECT_EQ(cvterm_table.name, CVTERM_TABLE);
    EXPECT_EQ(cvterm_table.primary_key, CVTERM_PK);
    EXPECT_TRUE(cvterm_table.foreign_keys.empty());

    Table feature_table = wrapper->get_table(FEATURE_TABLE);

    EXPECT_EQ(feature_table.name, FEATURE_TABLE);
    EXPECT_EQ(feature_table.primary_key, FEATURE_PK);
    EXPECT_EQ(feature_table.foreign_keys.size(), 2);

    bool has_organism_fk = false;
    bool has_type_fk = false;
    for (const auto& fk : feature_table.foreign_keys) {
        if (fk.find("organism_id") != string::npos && fk.find("public.organism") != string::npos) {
            has_organism_fk = true;
        }
        if (fk.find("type_id") != string::npos && fk.find("public.cvterm") != string::npos) {
            has_type_fk = true;
        }
    }
    EXPECT_TRUE(has_organism_fk);
    EXPECT_TRUE(has_type_fk);
}

// map_table - SQL2ATOMS
TEST_F(PostgresWrapperTest, MapTablesFirstRowAtoms) {
    auto conn = create_db_connection();
    auto wrapper = create_wrapper(*conn);

    Table organism_table = wrapper->get_table(ORGANISM_TABLE);
    EXPECT_NO_THROW({ wrapper->map_table(organism_table, {"organism_id = 1"}, {}, false); });
    EXPECT_EQ(wrapper->mapper_handle_trie_size(), 34);

    Table feature_table = wrapper->get_table(FEATURE_TABLE);
    EXPECT_NO_THROW({ wrapper->map_table(feature_table, {"feature_id = 1"}, {}, false); });
    EXPECT_EQ(wrapper->mapper_handle_trie_size(), 81);

    Table cvterm_table = wrapper->get_table(CVTERM_TABLE);
    EXPECT_NO_THROW({ wrapper->map_table(cvterm_table, {"cvterm_id = 1"}, {}, false); });
    EXPECT_EQ(wrapper->mapper_handle_trie_size(), 101);
}

TEST_F(PostgresWrapperTest, MapTableWithClausesAndSkipColumnsAtoms) {
    auto conn = create_db_connection();
    auto wrapper = create_wrapper(*conn);

    Table table = wrapper->get_table(FEATURE_TABLE);
    vector<string> clauses = {"organism_id = " + to_string(DROSOPHILA_ORGANISM_ID), "feature_id <= 5"};
    vector<string> skip_columns = {"residues", "md5checksum", "seqlen"};

    EXPECT_NO_THROW({ wrapper->map_table(table, clauses, skip_columns, false); });
    EXPECT_EQ(wrapper->mapper_handle_trie_size(), 114);
}

TEST_F(PostgresWrapperTest, MapTableZeroRowsAtoms) {
    auto conn = create_db_connection();
    auto wrapper = create_wrapper(*conn);

    Table table = wrapper->get_table(FEATURE_TABLE);
    vector<string> clauses = {"feature_id = -999"};

    EXPECT_NO_THROW({ wrapper->map_table(table, clauses, {}, false); });
    EXPECT_EQ(wrapper->mapper_handle_trie_size(), 0);
}

TEST_F(PostgresWrapperTest, MapTableWithNonExistentSkipColumnAtoms) {
    auto conn = create_db_connection();
    auto wrapper = create_wrapper(*conn);

    Table table = wrapper->get_table(FEATURE_TABLE);

    vector<string> clauses = {"feature_id < 10"};
    vector<string> skip_columns = {"column_xyz"};

    EXPECT_THROW({ wrapper->map_table(table, clauses, skip_columns, false); }, std::runtime_error);
    EXPECT_EQ(wrapper->mapper_handle_trie_size(), 0);
}

TEST_F(PostgresWrapperTest, MapTableWithInvalidClauseAtoms) {
    auto conn = create_db_connection();
    auto wrapper = create_wrapper(*conn);

    Table table = wrapper->get_table(FEATURE_TABLE);

    vector<string> clauses = {"INVALID CLAUSE SYNTAX !!!"};

    EXPECT_THROW({ wrapper->map_table(table, clauses, {}, false); }, std::runtime_error);
    EXPECT_EQ(wrapper->mapper_handle_trie_size(), 0);
}

// map_table - SQL2METTA
TEST_F(PostgresWrapperTest, MapTablesFirstRowMetta) {
    auto conn = create_db_connection();
    auto wrapper = create_wrapper(*conn, MAPPER_TYPE::SQL2METTA);

    Table organism_table = wrapper->get_table(ORGANISM_TABLE);
    EXPECT_NO_THROW({ wrapper->map_table(organism_table, {"organism_id = 1"}, {}, false); });
    EXPECT_EQ(wrapper->mapper_handle_trie_size(), 19);

    Table feature_table = wrapper->get_table(FEATURE_TABLE);
    EXPECT_NO_THROW({ wrapper->map_table(feature_table, {"feature_id = 1"}, {}, false); });
    EXPECT_EQ(wrapper->mapper_handle_trie_size(), 51);

    Table cvterm_table = wrapper->get_table(CVTERM_TABLE);
    EXPECT_NO_THROW({ wrapper->map_table(cvterm_table, {"cvterm_id = 1"}, {}, false); });
    EXPECT_EQ(wrapper->mapper_handle_trie_size(), 65);
}

TEST_F(PostgresWrapperTest, MapTableWithClausesAndSkipColumnsMetta) {
    auto conn = create_db_connection();
    auto wrapper = create_wrapper(*conn, MAPPER_TYPE::SQL2METTA);

    Table table = wrapper->get_table(FEATURE_TABLE);
    vector<string> clauses = {"organism_id = " + to_string(DROSOPHILA_ORGANISM_ID), "feature_id <= 5"};
    vector<string> skip_columns = {"residues", "md5checksum", "seqlen"};

    EXPECT_NO_THROW({ wrapper->map_table(table, clauses, skip_columns, false); });
    EXPECT_EQ(wrapper->mapper_handle_trie_size(), 86);
}

TEST_F(PostgresWrapperTest, MapTableZeroRowsMetta) {
    auto conn = create_db_connection();
    auto wrapper = create_wrapper(*conn, MAPPER_TYPE::SQL2METTA);

    Table table = wrapper->get_table(FEATURE_TABLE);
    vector<string> clauses = {"feature_id = -999"};

    EXPECT_NO_THROW({ wrapper->map_table(table, clauses, {}, false); });
    EXPECT_EQ(wrapper->mapper_handle_trie_size(), 0);
}

TEST_F(PostgresWrapperTest, MapTableWithNonExistentSkipColumnMetta) {
    auto conn = create_db_connection();
    auto wrapper = create_wrapper(*conn, MAPPER_TYPE::SQL2METTA);

    Table table = wrapper->get_table(FEATURE_TABLE);

    vector<string> clauses = {"feature_id < 10"};
    vector<string> skip_columns = {"column_xyz"};

    EXPECT_THROW({ wrapper->map_table(table, clauses, skip_columns, false); }, std::runtime_error);
    EXPECT_EQ(wrapper->mapper_handle_trie_size(), 0);
}

TEST_F(PostgresWrapperTest, MapTableWithInvalidClauseMetta) {
    auto conn = create_db_connection();
    auto wrapper = create_wrapper(*conn, MAPPER_TYPE::SQL2METTA);

    Table table = wrapper->get_table(FEATURE_TABLE);

    vector<string> clauses = {"INVALID CLAUSE SYNTAX !!!"};

    EXPECT_THROW({ wrapper->map_table(table, clauses, {}, false); }, std::runtime_error);
    EXPECT_EQ(wrapper->mapper_handle_trie_size(), 0);
}

// map_sql_query - SQL2ATOMS
TEST_F(PostgresWrapperTest, MapSqlQueryFirstRowAtoms) {
    auto conn = create_db_connection();
    auto wrapper = create_wrapper(*conn);

    string query_organism = R"(
        SELECT
            o.organism_id AS public_organism__organism_id,
            o.genus AS public_organism__genus,
            o.species AS public_organism__species,
            o.common_name AS public_organism__common_name,
            o.abbreviation AS public_organism__abbreviation,
            o.comment AS public_organism__comment
        FROM organism AS o
        WHERE o.organism_id = 1
    )";

    EXPECT_NO_THROW({ wrapper->map_sql_query("test_organism", query_organism); });
    EXPECT_EQ(wrapper->mapper_handle_trie_size(), 34);

    string query_feature = R"(
        SELECT
            f.feature_id AS public_feature__feature_id,
            f.organism_id AS public_feature__organism_id,
            f.type_id AS public_feature__type_id,
            f.name AS public_feature__name,
            f.uniquename AS public_feature__uniquename,
            f.residues AS public_feature__residues,
            f.md5checksum AS public_feature__md5checksum,
            f.seqlen AS public_feature__seqlen,
            f.is_analysis AS public_feature__is_analysis,
            f.is_obsolete AS public_feature__is_obsolete
        FROM feature AS f
        WHERE f.feature_id = 1
    )";

    EXPECT_NO_THROW({ wrapper->map_sql_query("test_feature", query_feature); });
    EXPECT_EQ(wrapper->mapper_handle_trie_size(), 81);

    string query_cvterm = R"(
        SELECT
            c.cvterm_id AS public_cvterm__cvterm_id,
            c.name AS public_cvterm__name,
            c.definition AS public_cvterm__definition,
            c.is_obsolete AS public_cvterm__is_obsolete,
            c.is_relationshiptype AS public_cvterm__is_relationshiptype
        FROM cvterm AS c
        WHERE c.cvterm_id = 1
    )";

    EXPECT_NO_THROW({ wrapper->map_sql_query("test_cvterm", query_cvterm); });
    EXPECT_EQ(wrapper->mapper_handle_trie_size(), 101);
}

TEST_F(PostgresWrapperTest, MapSqlQueryWithClausesAndSkipColumnsAtoms) {
    auto conn = create_db_connection();
    auto wrapper = create_wrapper(*conn);

    string query = R"(
        SELECT
            f.feature_id AS public_feature__feature_id,
            f.organism_id AS public_feature__organism_id,
            f.type_id AS public_feature__type_id,
            f.name AS public_feature__name,
            f.uniquename AS public_feature__uniquename,
            f.is_analysis AS public_feature__is_analysis,
            f.is_obsolete AS public_feature__is_obsolete
        FROM feature AS f
        WHERE f.organism_id = )" +
                   to_string(DROSOPHILA_ORGANISM_ID) + R"( AND f.feature_id <= 5)";

    EXPECT_NO_THROW({ wrapper->map_sql_query("test_feature_clause_and_skip", query); });
    EXPECT_EQ(wrapper->mapper_handle_trie_size(), 114);
}

TEST_F(PostgresWrapperTest, MapSqlQueryZeroRowsAtoms) {
    auto conn = create_db_connection();
    auto wrapper = create_wrapper(*conn);

    string query = R"(
        SELECT
            f.feature_id AS public_feature__feature_id,
            f.organism_id AS public_feature__organism_id,
            f.type_id AS public_feature__type_id,
            f.name AS public_feature__name,
            f.uniquename AS public_feature__uniquename,
            f.residues AS public_feature__residues,
            f.md5checksum AS public_feature__md5checksum,
            f.seqlen AS public_feature__seqlen,
            f.is_analysis AS public_feature__is_analysis,
            f.is_obsolete AS public_feature__is_obsolete
        FROM feature AS f
        WHERE f.feature_id = -999
    )";

    EXPECT_NO_THROW({ wrapper->map_sql_query("test_feature_zero_rows", query); });
    EXPECT_EQ(wrapper->mapper_handle_trie_size(), 0);
}

TEST_F(PostgresWrapperTest, MapSqlQueryWithNonExistentSkipColumnAtoms) {
    auto conn = create_db_connection();
    auto wrapper = create_wrapper(*conn);

    string query = R"(
        SELECT
            f.feature_id AS public_feature__feature_id,
            f.organism_id AS public_feature__organism_id,
            f.type_id AS public_feature__type_id,
            f.name AS public_feature__name,
            f.uniquename AS public_feature__uniquename,
            f.column_xyz AS public_feature__column_xyz
        FROM feature AS f
        WHERE f.feature_id < 10
    )";

    EXPECT_THROW({ wrapper->map_sql_query("test_feature_with_non_existent_column", query); },
                 std::runtime_error);
    EXPECT_EQ(wrapper->mapper_handle_trie_size(), 0);
}

TEST_F(PostgresWrapperTest, MapSqlQueryWithInvalidClauseAtoms) {
    auto conn = create_db_connection();
    auto wrapper = create_wrapper(*conn);

    string query = R"(
        SELECT
            f.feature_id AS public_feature__feature_id,
            f.organism_id AS public_feature__organism_id,
            f.type_id AS public_feature__type_id,
            f.name AS public_feature__name,
            f.uniquename AS public_feature__uniquename,
            f.residues AS public_feature__residues,
            f.md5checksum AS public_feature__md5checksum,
            f.seqlen AS public_feature__seqlen,
            f.is_analysis AS public_feature__is_analysis,
            f.is_obsolete AS public_feature__is_obsolete
        FROM feature AS f
        WHERE INVALID CLAUSE SYNTAX !!!
    )";

    EXPECT_THROW({ wrapper->map_sql_query("test_feature_with_invalid_clause", query); },
                 std::runtime_error);
    EXPECT_EQ(wrapper->mapper_handle_trie_size(), 0);
}

// map_sql_query - SQL2METTA
TEST_F(PostgresWrapperTest, MapSqlQueryFirstRowMetta) {
    auto conn = create_db_connection();
    auto wrapper = create_wrapper(*conn, MAPPER_TYPE::SQL2METTA);

    string query_organism = R"(
        SELECT
            o.organism_id AS public_organism__organism_id,
            o.genus AS public_organism__genus,
            o.species AS public_organism__species,
            o.common_name AS public_organism__common_name,
            o.abbreviation AS public_organism__abbreviation,
            o.comment AS public_organism__comment
        FROM organism AS o
        WHERE o.organism_id = 1
    )";

    EXPECT_NO_THROW({ wrapper->map_sql_query("test_organism", query_organism); });
    EXPECT_EQ(wrapper->mapper_handle_trie_size(), 19);

    string query_feature = R"(
        SELECT
            f.feature_id AS public_feature__feature_id,
            f.organism_id AS public_feature__organism_id,
            f.type_id AS public_feature__type_id,
            f.name AS public_feature__name,
            f.uniquename AS public_feature__uniquename,
            f.residues AS public_feature__residues,
            f.md5checksum AS public_feature__md5checksum,
            f.seqlen AS public_feature__seqlen,
            f.is_analysis AS public_feature__is_analysis,
            f.is_obsolete AS public_feature__is_obsolete
        FROM feature AS f
        WHERE f.feature_id = 1
    )";

    EXPECT_NO_THROW({ wrapper->map_sql_query("test_feature", query_feature); });
    EXPECT_EQ(wrapper->mapper_handle_trie_size(), 51);

    string query_cvterm = R"(
        SELECT
            c.cvterm_id AS public_cvterm__cvterm_id,
            c.name AS public_cvterm__name,
            c.definition AS public_cvterm__definition,
            c.is_obsolete AS public_cvterm__is_obsolete,
            c.is_relationshiptype AS public_cvterm__is_relationshiptype
        FROM cvterm AS c
        WHERE c.cvterm_id = 1
    )";

    EXPECT_NO_THROW({ wrapper->map_sql_query("test_cvterm", query_cvterm); });
    EXPECT_EQ(wrapper->mapper_handle_trie_size(), 65);
}

TEST_F(PostgresWrapperTest, MapSqlQueryWithClausesAndSkipColumnsMetta) {
    auto conn = create_db_connection();
    auto wrapper = create_wrapper(*conn, MAPPER_TYPE::SQL2METTA);

    string query = R"(
        SELECT
            f.feature_id AS public_feature__feature_id,
            f.organism_id AS public_feature__organism_id,
            f.type_id AS public_feature__type_id,
            f.name AS public_feature__name,
            f.uniquename AS public_feature__uniquename,
            f.is_analysis AS public_feature__is_analysis,
            f.is_obsolete AS public_feature__is_obsolete
        FROM feature AS f
        WHERE f.organism_id = )" +
                   to_string(DROSOPHILA_ORGANISM_ID) + R"( AND f.feature_id <= 5)";

    EXPECT_NO_THROW({ wrapper->map_sql_query("test_feature_clause_and_skip", query); });
    EXPECT_EQ(wrapper->mapper_handle_trie_size(), 86);
}

TEST_F(PostgresWrapperTest, MapSqlQueryZeroRowsMetta) {
    auto conn = create_db_connection();
    auto wrapper = create_wrapper(*conn, MAPPER_TYPE::SQL2METTA);

    string query = R"(
        SELECT
            f.feature_id AS public_feature__feature_id,
            f.organism_id AS public_feature__organism_id,
            f.type_id AS public_feature__type_id,
            f.name AS public_feature__name,
            f.uniquename AS public_feature__uniquename,
            f.residues AS public_feature__residues,
            f.md5checksum AS public_feature__md5checksum,
            f.seqlen AS public_feature__seqlen,
            f.is_analysis AS public_feature__is_analysis,
            f.is_obsolete AS public_feature__is_obsolete
        FROM feature AS f
        WHERE f.feature_id = -999
    )";

    EXPECT_NO_THROW({ wrapper->map_sql_query("test_feature_zero_rows", query); });
    EXPECT_EQ(wrapper->mapper_handle_trie_size(), 0);
}

TEST_F(PostgresWrapperTest, MapSqlQueryWithNonExistentSkipColumnMetta) {
    auto conn = create_db_connection();
    auto wrapper = create_wrapper(*conn, MAPPER_TYPE::SQL2METTA);

    string query = R"(
        SELECT
            f.feature_id AS public_feature__feature_id,
            f.organism_id AS public_feature__organism_id,
            f.type_id AS public_feature__type_id,
            f.name AS public_feature__name,
            f.uniquename AS public_feature__uniquename,
            f.column_xyz AS public_feature__column_xyz
        FROM feature AS f
        WHERE f.feature_id < 10
    )";

    EXPECT_THROW({ wrapper->map_sql_query("test_feature_with_non_existent_column", query); },
                 std::runtime_error);
    EXPECT_EQ(wrapper->mapper_handle_trie_size(), 0);
}

TEST_F(PostgresWrapperTest, MapSqlQueryWithInvalidClauseMetta) {
    auto conn = create_db_connection();
    auto wrapper = create_wrapper(*conn, MAPPER_TYPE::SQL2METTA);

    string query = R"(
        SELECT
            f.feature_id AS public_feature__feature_id,
            f.organism_id AS public_feature__organism_id,
            f.type_id AS public_feature__type_id,
            f.name AS public_feature__name,
            f.uniquename AS public_feature__uniquename,
            f.residues AS public_feature__residues,
            f.md5checksum AS public_feature__md5checksum,
            f.seqlen AS public_feature__seqlen,
            f.is_analysis AS public_feature__is_analysis,
            f.is_obsolete AS public_feature__is_obsolete
        FROM feature AS f
        WHERE INVALID CLAUSE SYNTAX !!!
    )";

    EXPECT_THROW({ wrapper->map_sql_query("test_feature_with_invalid_clause", query); },
                 std::runtime_error);
    EXPECT_EQ(wrapper->mapper_handle_trie_size(), 0);
}

TEST_F(PostgresWrapperTest, MapTablesFirstRowAtomsWithContextFile) {
    vector<TableMapping> tables_mapping;

    if (!load_context_file("/tmp/context.json", tables_mapping)) {
        Utils::error("Failed to load context file. Aborting test.");
    }

    auto conn = create_db_connection();
    auto wrapper = create_wrapper(*conn);

    vector<unsigned int> atoms_sizes;

    for (const auto& tm : tables_mapping) {
        if (!tm.query.has_value()) {
            string table_name = tm.table_name;
            vector<string> skip_columns = tm.skip_columns.value_or(vector<string>{});
            vector<string> where_clauses = tm.where_clauses.value_or(vector<string>{});

            Table table = wrapper->get_table(table_name);
            EXPECT_NO_THROW({ wrapper->map_table(table, where_clauses, skip_columns, false); });
            atoms_sizes.push_back(wrapper->mapper_handle_trie_size());
        }
    }
    EXPECT_EQ(atoms_sizes.size(), 3);
    EXPECT_EQ(atoms_sizes[0], 34);
    EXPECT_EQ(atoms_sizes[1], 81);
    EXPECT_EQ(atoms_sizes[2], 101);
}

TEST_F(PostgresWrapperTest, PipelineProcessor) {
    string query_organism = R"(
        SELECT
            o.organism_id AS public_organism__organism_id,
            o.genus AS public_organism__genus,
            o.species AS public_organism__species,
            o.common_name AS public_organism__common_name,
            o.abbreviation AS public_organism__abbreviation,
            o.comment AS public_organism__comment
        FROM organism AS o
        WHERE o.organism_id = 1
    )";

    auto queue = make_shared<SharedQueue>();

    DatabaseMappingJob db_job(
        TEST_HOST, TEST_PORT, TEST_DB, TEST_USER, TEST_PASSWORD, MAPPER_TYPE::SQL2ATOMS, queue);
    db_job.add_task_query("Test1", query_organism);

    auto producer = make_shared<DedicatedThread>("producer", &db_job);

    EXPECT_EQ(queue->size(), 0);
    producer->setup();
    EXPECT_EQ(queue->size(), 0);
    producer->start();
    EXPECT_EQ(queue->size(), 0);

    while (!db_job.is_finished()) {
        Utils::sleep();
    }
    producer->stop();

    EXPECT_EQ(queue->size(), 34);

    AtomPersistenceJob atomdb_job(queue);
    auto consumer = make_shared<DedicatedThread>("consumer", &atomdb_job);

    EXPECT_EQ(queue->size(), 34);
    consumer->setup();
    EXPECT_EQ(queue->size(), 34);
    consumer->start();
    EXPECT_EQ(queue->size(), 34);

    while (!atomdb_job.is_finished()) {
        Utils::sleep();
    }
    consumer->stop();

    EXPECT_EQ(queue->size(), 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new PostgresWrapperTestEnvironment);
    return RUN_ALL_TESTS();
}
