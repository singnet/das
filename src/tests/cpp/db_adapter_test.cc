#include <gtest/gtest.h>

#include <atomic>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "PostgresWrapper.h"
#include "TestConfig.h"

using namespace std;

class PostgresTestEnvironment : public ::testing::Environment {
   public:
    void SetUp() override { TestConfig::load_environment(); }

    void TearDown() override {}
};

class PostgresTest : public ::testing::Test {
   protected:
    void SetUp() override {
        host = "chado.flybase.org";
        port = 5432;
        database = "flybase";
        user = "flybase";
        // password = "password"; // No password needed for this public database
    }

    void TearDown() override {}

    string host;
    int port;
    string database;
    string user;
    string password;
};

TEST_F(PostgresTest, ConnectSuccess) {
    PostgresWrapper wrapper(host, port, database, user, password);

    auto conn = wrapper.connect();

    ASSERT_NE(conn, nullptr);
    EXPECT_TRUE(conn->is_open());
}

TEST_F(PostgresTest, SimpleQuery) {
    PostgresWrapper wrapper(host, port, database, user, password);

    auto conn = wrapper.connect();
    ASSERT_NE(conn, nullptr);
    ASSERT_TRUE(conn->is_open());

    // Default transaction (write-read)
    pqxx::work transaction(*conn);
    string query = "select * from feature f where f.feature_id = 3164551;";
    pqxx::result result = transaction.exec(query);
    transaction.commit();

    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0]["uniquename"].as<string>(), "FBgn0020238");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new PostgresTestEnvironment);
    return RUN_ALL_TESTS();
}