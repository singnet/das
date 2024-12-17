#include "gtest/gtest.h"

#include "Utils.h"
#include "QueryNode.h"
#include "QueryAnswer.h"

using namespace commons;
using namespace query_node;

TEST(QueryNode, basics) {
    
    string server_id = "server";
    string client1_id = "client1";
    string client2_id = "client2";

    QueryNodeServer server(server_id);
    QueryNodeClient client1(client1_id, server_id);
    QueryNodeClient client2(client2_id, server_id);

    EXPECT_TRUE(server.is_query_answers_empty());
    EXPECT_FALSE(server.is_query_answers_finished());
    EXPECT_TRUE(client1.is_query_answers_empty());
    EXPECT_FALSE(client1.is_query_answers_finished());
    EXPECT_TRUE(client2.is_query_answers_empty());
    EXPECT_FALSE(client2.is_query_answers_finished());

    ASSERT_TRUE(server.pop_query_answer() == (QueryAnswer *) 0);

    client1.add_query_answer((QueryAnswer *) 1);
    client1.add_query_answer((QueryAnswer *) 2);
    Utils::sleep(1000);
    client2.add_query_answer((QueryAnswer *) 3);
    client2.add_query_answer((QueryAnswer *) 4);
    Utils::sleep(1000);
    client1.add_query_answer((QueryAnswer *) 5);
    Utils::sleep(1000);

    EXPECT_FALSE(server.is_query_answers_empty());
    EXPECT_FALSE(server.is_query_answers_finished());
    EXPECT_TRUE(client1.is_query_answers_empty());
    EXPECT_FALSE(client1.is_query_answers_finished());
    EXPECT_TRUE(client2.is_query_answers_empty());
    EXPECT_FALSE(client2.is_query_answers_finished());

    client1.query_answers_finished();
    client2.query_answers_finished();
    Utils::sleep(1000);

    EXPECT_FALSE(server.is_query_answers_empty());
    EXPECT_TRUE(server.is_query_answers_finished());
    EXPECT_TRUE(client1.is_query_answers_empty());
    EXPECT_TRUE(client1.is_query_answers_finished());
    EXPECT_TRUE(client2.is_query_answers_empty());
    EXPECT_TRUE(client2.is_query_answers_finished());

    ASSERT_TRUE(server.pop_query_answer() == (QueryAnswer *) 1);
    ASSERT_TRUE(server.pop_query_answer() == (QueryAnswer *) 2);
    ASSERT_TRUE(server.pop_query_answer() == (QueryAnswer *) 3);
    ASSERT_TRUE(server.pop_query_answer() == (QueryAnswer *) 4);
    ASSERT_TRUE(server.pop_query_answer() == (QueryAnswer *) 5);
    ASSERT_TRUE(server.pop_query_answer() == (QueryAnswer *) 0);

    EXPECT_TRUE(server.is_query_answers_empty());
    EXPECT_TRUE(server.is_query_answers_finished());
    EXPECT_TRUE(client1.is_query_answers_empty());
    EXPECT_TRUE(client1.is_query_answers_finished());
    EXPECT_TRUE(client2.is_query_answers_empty());
    EXPECT_TRUE(client2.is_query_answers_finished());
}
