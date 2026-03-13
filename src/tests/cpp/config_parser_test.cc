#include <gtest/gtest.h>

#include <stdexcept>

#include "JsonConfig.h"
#include "JsonConfigParser.h"
#include "Properties.h"

using namespace commons;

namespace {

const char* kValidConfigV2 = R"({
  "schema_version": "2.0",
  "atomdb": {
    "type": "redismongodb",
    "redis": {
      "port": 40020,
      "cluster": false
    },
    "mongodb": {
      "port": 40021,
      "username": "admin"
    }
  },
  "loaders": {
    "metta": { "image": "trueagi/das:metta-parser" }
  },
  "agents": {
    "query": { "port": 40002 }
  },
  "brokers": {
    "attention": { "port": 40001 }
  },
  "params": {
    "query": { "max_answers": 100 }
  }
})";

}  // namespace

TEST(ConfigParserTest, LoadFromStringValidSchemaV2) {
    JsonConfig config = JsonConfigParser::load_from_string(kValidConfigV2);
    EXPECT_EQ(config.get_schema_version(), "2.0");
}

TEST(ConfigParserTest, ToPropertiesNestedStructure) {
    JsonConfig config = JsonConfigParser::load_from_string(kValidConfigV2);
    Properties props = config.to_properties();

    // Top-level scalar
    const std::string* schema = props.get_ptr<std::string>("schema_version");
    ASSERT_NE(schema, nullptr);
    EXPECT_EQ(*schema, "2.0");

    // Nested sections
    auto atomdb = props.get_nested("atomdb");
    ASSERT_NE(atomdb, nullptr);
    const std::string* type = atomdb->get_ptr<std::string>("type");
    ASSERT_NE(type, nullptr);
    EXPECT_EQ(*type, "redismongodb");

    auto redis = atomdb->get_nested("redis");
    ASSERT_NE(redis, nullptr);
    const long* port = redis->get_ptr<long>("port");
    ASSERT_NE(port, nullptr);
    EXPECT_EQ(*port, 40020);
    const bool* cluster = redis->get_ptr<bool>("cluster");
    ASSERT_NE(cluster, nullptr);
    EXPECT_FALSE(*cluster);

    auto mongodb = atomdb->get_nested("mongodb");
    ASSERT_NE(mongodb, nullptr);
    EXPECT_EQ(mongodb->get_or<long>("port", 0), 40021);
    EXPECT_EQ(mongodb->get_or<std::string>("username", ""), "admin");
}

TEST(ConfigParserTest, ToPropertiesNestedAgentsAndParams) {
    JsonConfig config = JsonConfigParser::load_from_string(kValidConfigV2);
    Properties props = config.to_properties();

    auto agents = props.get_nested("agents");
    ASSERT_NE(agents, nullptr);
    auto query = agents->get_nested("query");
    ASSERT_NE(query, nullptr);
    EXPECT_EQ(query->get_or<long>("port", 0), 40002);

    auto params = props.get_nested("params");
    ASSERT_NE(params, nullptr);
    auto query_params = params->get_nested("query");
    ASSERT_NE(query_params, nullptr);
    EXPECT_EQ(query_params->get_or<long>("max_answers", 0), 100);
}

TEST(ConfigParserTest, GetNestedMissingKeyReturnsNullptr) {
    JsonConfig config = JsonConfigParser::load_from_string(kValidConfigV2);
    Properties props = config.to_properties();

    EXPECT_EQ(props.get_nested("nonexistent"), nullptr);
    auto atomdb = props.get_nested("atomdb");
    ASSERT_NE(atomdb, nullptr);
    EXPECT_EQ(atomdb->get_nested("nonexistent"), nullptr);
}

TEST(ConfigParserTest, MissingSchemaVersionThrows) {
    const char* no_version =
        R"({ "atomdb": {}, "loaders": {}, "agents": {}, "brokers": {}, "params": {} })";
    EXPECT_THROW(JsonConfigParser::load_from_string(no_version), std::runtime_error);
}

TEST(ConfigParserTest, UnsupportedSchemaVersionThrows) {
    const char* bad_version = R"({
      "schema_version": "99.0",
      "atomdb": {}, "loaders": {}, "agents": {}, "brokers": {}, "params": {}
    })";
    EXPECT_THROW(JsonConfigParser::load_from_string(bad_version), std::runtime_error);
}

TEST(ConfigParserTest, MissingRequiredFieldThrows) {
    const char* no_atomdb = R"({
      "schema_version": "2.0",
      "loaders": {}, "agents": {}, "brokers": {}, "params": {}
    })";
    EXPECT_THROW(JsonConfigParser::load_from_string(no_atomdb), std::runtime_error);
}

TEST(ConfigParserTest, NullRequiredFieldThrows) {
    const char* null_atomdb = R"({
      "schema_version": "2.0",
      "atomdb": null,
      "loaders": {}, "agents": {}, "brokers": {}, "params": {}
    })";
    EXPECT_THROW(JsonConfigParser::load_from_string(null_atomdb), std::runtime_error);
}

TEST(ConfigParserTest, InvalidJsonThrows) {
    EXPECT_THROW(JsonConfigParser::load_from_string("{ invalid json }"), std::runtime_error);
}

TEST(ConfigParserTest, GetJsonReturnsRoot) {
    JsonConfig config = JsonConfigParser::load_from_string(kValidConfigV2);
    const auto& j = config.get_json();
    EXPECT_TRUE(j.is_object());
    EXPECT_EQ(j["schema_version"].get<std::string>(), "2.0");
    EXPECT_EQ(j["atomdb"]["redis"]["port"].get<long>(), 40020);
}
