#include <gtest/gtest.h>

#include <stdexcept>

#include "JsonConfig.h"
#include "JsonConfigParser.h"
#include "Properties.h"

using namespace std;
using namespace commons;

namespace {

const char* kValidConfigV1 = R"({
  "atomdb": {
    "type": "redismongodb",
    "redis": {
      "port": 40020,
      "hostname": "localhost",
      "cluster": false
    },
    "mongodb": {
      "port": 40021,
      "hostname": "localhost",
      "username": "admin",
      "password": "admin"
    },
    "morkdb": {
      "hostname": "localhost",
      "port": 40022
    }
  },
  "loaders": {
    "metta": { "image": "trueagi/das:metta-parser" }
  },
  "agents": {
    "query": { "endpoint": "localhost:40002", "params": { "max-answers": 100 } }
  },
  "brokers": {
    "attention": { "endpoint": "localhost:40001" }
  }
})";

}  // namespace

TEST(ConfigParserTest, GetNestedStructure) {
    JsonConfig config = JsonConfigParser::load_from_string(kValidConfigV1);

    // Nested sections
    auto atomdb = config.at_path("atomdb").get_or<JsonConfig>(JsonConfig());
    string type = atomdb.at_path("type").get_or<string>("");
    EXPECT_EQ(type, "redismongodb");

    auto redis = atomdb.at_path("redis").get_or<JsonConfig>(JsonConfig());
    long port = redis.at_path("port").get_or<long>(0);
    EXPECT_EQ(port, 40020);
    string hostname = redis.at_path("hostname").get_or<string>("");
    EXPECT_EQ(hostname, "localhost");
    bool cluster = redis.at_path("cluster").get_or<bool>(false);
    EXPECT_FALSE(cluster);

    auto mongodb = atomdb.at_path("mongodb").get_or<JsonConfig>(JsonConfig());
    port = mongodb.at_path("port").get_or<long>(0);
    EXPECT_EQ(port, 40021);
    hostname = mongodb.at_path("hostname").get_or<string>("");
    EXPECT_EQ(hostname, "localhost");
    string username = mongodb.at_path("username").get_or<string>("");
    EXPECT_EQ(username, "admin");
    string password = mongodb.at_path("password").get_or<string>("");
    EXPECT_EQ(password, "admin");

    auto morkdb = atomdb.at_path("morkdb").get_or<JsonConfig>(JsonConfig());
    hostname = morkdb.at_path("hostname").get_or<string>("");
    EXPECT_EQ(hostname, "localhost");
    port = morkdb.at_path("port").get_or<long>(0);
    EXPECT_EQ(port, 40022);
}

TEST(ConfigParserTest, GetNestedAgentsAndParams) {
    JsonConfig config = JsonConfigParser::load_from_string(kValidConfigV1);
    string query = config.at_path("agents.query.endpoint").get_or<string>("");
    EXPECT_EQ(query, "localhost:40002");
}

TEST(ConfigParserTest, GetParamsFromDasConfig) {
    JsonConfig config = JsonConfigParser::load_from_string(kValidConfigV1);
    long max_answers = config.at_path("agents.query.params.max-answers").get_or<long>(0);
    EXPECT_EQ(max_answers, 100);
}

TEST(ConfigParserTest, GetNestedMissingKeyReturnsEmptyString) {
    JsonConfig config = JsonConfigParser::load_from_string(kValidConfigV1);
    EXPECT_EQ(config.at_path("nonexistent").get_or<string>(""), "");
    EXPECT_EQ(config.at_path("atomdb.nonexistent").get_or<string>(""), "");
}

TEST(ConfigParserTest, InvalidJsonThrows) {
    EXPECT_THROW(JsonConfigParser::load_from_string("{ invalid json }"), runtime_error);
}

TEST(ConfigParserTest, GetJsonReturnsRoot) {
    JsonConfig config = JsonConfigParser::load_from_string(kValidConfigV1);
    const auto& j = config.get_json();
    EXPECT_TRUE(j.is_object());
    EXPECT_EQ(j["atomdb"]["redis"]["port"].get<long>(), 40020);
}
