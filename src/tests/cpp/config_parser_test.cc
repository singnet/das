#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <unordered_map>

#include "JsonConfig.h"
#include "JsonConfigParser.h"
#include "VaultJsonResolver.h"

using namespace std;
using namespace commons;
using namespace vault;

namespace {

const char* kValidConfigV1 = R"({
  "atomdb": {
    "type": "redismongodb",
    "composite_type_enabled": true,
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
  },
  "vault": {
    "type": "openbao",
    "endpoint": "localhost:8200"
  }
})";

}  // namespace

TEST(ConfigParserTest, GetNestedStructure) {
    JsonConfig config = JsonConfigParser::load_from_string(kValidConfigV1);

    auto atomdb = config.at_path("atomdb").get_or<JsonConfig>(JsonConfig());
    string type = atomdb.at_path("type").get_or<string>("");
    EXPECT_EQ(type, "redismongodb");
    bool composite_type_enabled = atomdb.at_path("composite_type_enabled").get_or<bool>(false);
    EXPECT_TRUE(composite_type_enabled);

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

TEST(ConfigParserTest, MissingVaultFieldThrows) {
    string without_vault = R"({
      "atomdb": { "type": "redismongodb" },
      "loaders": {},
      "agents": {}
    })";
    EXPECT_THROW(JsonConfigParser::load_from_string(without_vault), runtime_error);
}

TEST(ConfigParserTest, VaultTypeValueNotCheckedWithoutRefs) {
    string other_type = R"({
      "atomdb": { "type": "redismongodb" },
      "loaders": {},
      "agents": {},
      "vault": { "type": "hashicorp", "endpoint": "localhost:8200" }
    })";
    JsonConfig config = JsonConfigParser::load_from_string(other_type);
    EXPECT_EQ(config.at_path("vault.type").get_or<string>(""), "hashicorp");
}

TEST(ConfigParserTest, NoVaultRefsDoesNotRequireToken) {
    // No vault:// strings -> OpenBao is not contacted; env token not needed.
    JsonConfig config = JsonConfigParser::load_from_string(kValidConfigV1);
    EXPECT_EQ(config.at_path("vault.type").get_or<string>(""), "openbao");
    EXPECT_EQ(config.at_path("atomdb.mongodb.username").get_or<string>(""), "admin");
}

TEST(VaultJsonResolverTest, ParseUri) {
    auto parsed = VaultJsonResolver::parse_uri("vault://test/dbs/mongodb_user");
    EXPECT_EQ(parsed.first, "test");
    EXPECT_EQ(parsed.second, "dbs/mongodb_user");
}

TEST(VaultJsonResolverTest, ParseUriRejectsMissingPath) {
    EXPECT_THROW(VaultJsonResolver::parse_uri("vault://test"), runtime_error);
    EXPECT_THROW(VaultJsonResolver::parse_uri("vault://"), runtime_error);
}

TEST(VaultJsonResolverTest, InjectSingleKeyUnwrapsValue) {
    nlohmann::json data = {{"value", "admin"}};
    EXPECT_EQ(VaultJsonResolver::inject_payload(data), "admin");

    nlohmann::json data2 = {{"username", "admin"}};
    EXPECT_EQ(VaultJsonResolver::inject_payload(data2), "admin");
}

TEST(VaultJsonResolverTest, InjectMultiKeyKeepsObject) {
    nlohmann::json data = {
        {"endpoint", "localhost:40021"}, {"username", "admin"}, {"password", "secret"}};
    auto injected = VaultJsonResolver::inject_payload(data);
    EXPECT_TRUE(injected.is_object());
    EXPECT_EQ(injected["username"], "admin");
    EXPECT_EQ(injected["password"], "secret");
}

TEST(VaultJsonResolverTest, ScalarAndObjectReplacement) {
    nlohmann::json root = {{"vault", {{"type", "openbao"}, {"endpoint", "localhost:8200"}}},
                           {"atomdb",
                            {{"mongodb",
                              {{"image", "mongodb/img"},
                               {"endpoint", "vault://test/dbs/mongodb_endpoint"},
                               {"username", "vault://test/dbs/mongodb_user"},
                               {"password", "vault://test/dbs/mongodb_pass"}}},
                             {"peer", "vault://test/dbs/mongodb"}}}};

    unordered_map<string, nlohmann::json> store = {
        {"test|dbs/mongodb_endpoint", {{"value", "localhost:40021"}}},
        {"test|dbs/mongodb_user", {{"username", "admin"}}},
        {"test|dbs/mongodb_pass", {{"password", "s3cret"}}},
        {"test|dbs/mongodb",
         {{"endpoint", "localhost:40021"}, {"username", "admin"}, {"password", "s3cret"}}},
    };

    VaultJsonResolver::resolve(root, [&store](const string& mount, const string& path) {
        string key = mount + "|" + path;
        auto it = store.find(key);
        if (it == store.end()) {
            throw runtime_error("missing secret: " + key);
        }
        return it->second;
    });

    EXPECT_EQ(root["atomdb"]["mongodb"]["image"], "mongodb/img");
    EXPECT_EQ(root["atomdb"]["mongodb"]["endpoint"], "localhost:40021");
    EXPECT_EQ(root["atomdb"]["mongodb"]["username"], "admin");
    EXPECT_EQ(root["atomdb"]["mongodb"]["password"], "s3cret");
    EXPECT_TRUE(root["atomdb"]["peer"].is_object());
    EXPECT_EQ(root["atomdb"]["peer"]["username"], "admin");
    // vault block untouched
    EXPECT_EQ(root["vault"]["endpoint"], "localhost:8200");
}

TEST(VaultJsonResolverTest, MissingSecretFails) {
    nlohmann::json root = {{"vault", {{"type", "openbao"}, {"endpoint", "localhost:8200"}}},
                           {"password", "vault://test/dbs/missing"}};

    EXPECT_THROW(VaultJsonResolver::resolve(root,
                                            [](const string&, const string&) -> nlohmann::json {
                                                throw runtime_error("not found");
                                            }),
                 runtime_error);
}

TEST(VaultJsonResolverTest, HasVaultRefsIgnoresVaultBlock) {
    nlohmann::json root = {{"vault", {{"type", "openbao"}, {"endpoint", "localhost:8200"}}},
                           {"x", "plain"}};
    EXPECT_FALSE(VaultJsonResolver::has_vault_refs(root));

    root["x"] = "vault://test/dbs/x";
    EXPECT_TRUE(VaultJsonResolver::has_vault_refs(root));
}

TEST(ConfigParserTest, VaultRefsWithoutTerminalTokenFail) {
    string with_ref = R"({
      "atomdb": {
        "type": "redismongodb",
        "mongodb": { "password": "vault://test/dbs/pass" }
      },
      "loaders": {},
      "agents": {},
      "vault": { "type": "openbao", "endpoint": "localhost:8200" }
    })";
    EXPECT_THROW(JsonConfigParser::load_from_string(with_ref), runtime_error);
}
