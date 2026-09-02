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
    "endpoint": "localhost:40010"
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
      "vault": { "type": "hashicorp", "endpoint": "localhost:40010" }
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

TEST(VaultJsonResolverTest, InjectPayloadPicksMatchingField) {
    nlohmann::json data = {
        {"endpoint", "localhost:40021"}, {"username", "admin"}, {"password", "secret"}};
    EXPECT_EQ(VaultJsonResolver::inject_payload(data, "username"), "admin");
    EXPECT_EQ(VaultJsonResolver::inject_payload(data, "password"), "secret");

    nlohmann::json wrapped = {{"mongodb", data}};
    auto injected = VaultJsonResolver::inject_payload(wrapped, "mongodb");
    EXPECT_TRUE(injected.is_object());
    EXPECT_EQ(injected["endpoint"], "localhost:40021");
    EXPECT_EQ(injected["username"], "admin");
}

TEST(VaultJsonResolverTest, InjectPayloadMissingFieldFails) {
    nlohmann::json data = {{"username", "admin"}};
    EXPECT_THROW(VaultJsonResolver::inject_payload(data, "password"), runtime_error);
    EXPECT_THROW(VaultJsonResolver::inject_payload(data, ""), runtime_error);
}

TEST(VaultJsonResolverTest, KeyMatchingReplacement) {
    nlohmann::json mongodb = {{"image", "mongodb/img"},
                              {"endpoint", "localhost:40021"},
                              {"username", "admin"},
                              {"password", "s3cret"}};
    nlohmann::json root = {{"vault", {{"type", "openbao"}, {"endpoint", "http://localhost:40010"}}},
                           {"atomdb",
                            {{"mongodb", "vault://test/db"},
                             {"peer",
                              {{"endpoint", "vault://test/db"},
                               {"username", "vault://test/db"},
                               {"password", "vault://test/db"}}}}}};

    unordered_map<string, nlohmann::json> store = {
        {"test|db",
         {{"mongodb", mongodb},
          {"endpoint", "localhost:40021"},
          {"username", "admin"},
          {"password", "s3cret"}}},
    };

    int fetches = 0;
    VaultJsonResolver::resolve(root, [&store, &fetches](const string& mount, const string& path) {
        fetches++;
        string key = mount + "|" + path;
        auto it = store.find(key);
        if (it == store.end()) {
            throw runtime_error("missing secret: " + key);
        }
        return it->second;
    });

    EXPECT_EQ(fetches, 1);
    EXPECT_EQ(root["atomdb"]["mongodb"]["image"], "mongodb/img");
    EXPECT_EQ(root["atomdb"]["mongodb"]["endpoint"], "localhost:40021");
    EXPECT_EQ(root["atomdb"]["mongodb"]["username"], "admin");
    EXPECT_EQ(root["atomdb"]["mongodb"]["password"], "s3cret");
    EXPECT_EQ(root["atomdb"]["peer"]["endpoint"], "localhost:40021");
    EXPECT_EQ(root["atomdb"]["peer"]["username"], "admin");
    EXPECT_EQ(root["atomdb"]["peer"]["password"], "s3cret");
    EXPECT_EQ(root["vault"]["endpoint"], "http://localhost:40010");
}

TEST(VaultJsonResolverTest, AttentionObjectFromMatchingKey) {
    nlohmann::json root = {{"vault", {{"type", "openbao"}, {"endpoint", "http://localhost:40010"}}},
                           {"agents", {{"attention", "vault://test/agents"}}}};
    VaultJsonResolver::resolve(root, [](const string&, const string&) {
        return nlohmann::json{{"attention", {{"endpoint", "localhost:40001"}}}};
    });
    EXPECT_TRUE(root["agents"]["attention"].is_object());
    EXPECT_EQ(root["agents"]["attention"]["endpoint"], "localhost:40001");
}

TEST(VaultJsonResolverTest, ArrayElementVaultRefFails) {
    nlohmann::json root = {{"vault", {{"type", "openbao"}, {"endpoint", "http://localhost:40010"}}},
                           {"nodes", nlohmann::json::array({"vault://test/db"})}};
    EXPECT_THROW(
        VaultJsonResolver::resolve(root,
                                   [](const string&, const string&) {
                                       return nlohmann::json{{"nodes", nlohmann::json::array()}};
                                   }),
        runtime_error);
}

TEST(VaultJsonResolverTest, MissingSecretFails) {
    nlohmann::json root = {{"vault", {{"type", "openbao"}, {"endpoint", "localhost:40010"}}},
                           {"password", "vault://test/dbs/missing"}};

    EXPECT_THROW(VaultJsonResolver::resolve(root,
                                            [](const string&, const string&) -> nlohmann::json {
                                                throw runtime_error("not found");
                                            }),
                 runtime_error);
}

TEST(VaultJsonResolverTest, HasVaultRefsIgnoresVaultBlock) {
    nlohmann::json root = {{"vault", {{"type", "openbao"}, {"endpoint", "localhost:40010"}}},
                           {"x", "plain"}};
    EXPECT_FALSE(VaultJsonResolver::has_vault_refs(root));

    root["x"] = "vault://test/dbs/x";
    EXPECT_TRUE(VaultJsonResolver::has_vault_refs(root));
}

namespace {

void expect_resolve_from_config_rejects_without_prompt(nlohmann::json root, const string& must_contain) {
    try {
        VaultJsonResolver::resolve_from_config(root);
        FAIL() << "expected resolve_from_config to throw";
    } catch (const runtime_error& e) {
        string msg = e.what();
        EXPECT_NE(msg.find(must_contain), string::npos) << msg;
        EXPECT_EQ(msg.find("stdin is not a TTY"), string::npos) << msg;
        EXPECT_EQ(msg.find("failed to read vault token"), string::npos) << msg;
    }
}

nlohmann::json vault_root_with(const nlohmann::json& extra) {
    nlohmann::json root = extra;
    root["vault"] = {{"type", "openbao"}, {"endpoint", "http://localhost:40010"}};
    return root;
}

}  // namespace

TEST(VaultJsonResolverTest, MalformedUriDoesNotPromptOrContactVault) {
    expect_resolve_from_config_rejects_without_prompt(vault_root_with({{"password", "vault://test"}}),
                                                      "vault://<mount>/<secret-path>");
}

TEST(VaultJsonResolverTest, ArrayVaultRefDoesNotPromptOrContactVault) {
    expect_resolve_from_config_rejects_without_prompt(
        vault_root_with({{"nodes", nlohmann::json::array({"vault://test/db"})}}),
        "not allowed as an array element");
}

TEST(VaultJsonResolverTest, EmptyConfigKeyDoesNotPromptOrContactVault) {
    nlohmann::json root = vault_root_with(nlohmann::json::object());
    root[""] = "vault://test/db";
    expect_resolve_from_config_rejects_without_prompt(root, "missing a DAS config file key");
}

TEST(VaultJsonResolverTest, MissingEndpointSchemeDoesNotPrompt) {
    nlohmann::json root = vault_root_with({{"password", "vault://test/db"}});
    root["vault"]["endpoint"] = "localhost:40010";
    expect_resolve_from_config_rejects_without_prompt(root, "must include a scheme");
}

TEST(VaultJsonResolverTest, UnsupportedEndpointSchemeDoesNotPrompt) {
    nlohmann::json root = vault_root_with({{"password", "vault://test/db"}});
    root["vault"]["endpoint"] = "ftp://localhost:40010";
    expect_resolve_from_config_rejects_without_prompt(root, "scheme must be http or https");

    root["vault"]["endpoint"] = "file:///tmp/vault";
    expect_resolve_from_config_rejects_without_prompt(root, "scheme must be http or https");
}

TEST(VaultJsonResolverTest, HttpAndHttpsEndpointsReachTokenPrompt) {
    nlohmann::json root = vault_root_with({{"password", "vault://test/db"}});
    for (const char* endpoint : {"http://localhost:40010", "https://localhost:40010"}) {
        root["vault"]["endpoint"] = endpoint;
        try {
            VaultJsonResolver::resolve_from_config(root);
            FAIL() << "expected token prompt failure for " << endpoint;
        } catch (const runtime_error& e) {
            string msg = e.what();
            EXPECT_NE(msg.find("stdin is not a TTY"), string::npos) << endpoint << ": " << msg;
            EXPECT_EQ(msg.find("scheme must be http or https"), string::npos) << endpoint << ": " << msg;
        }
    }
}

TEST(ConfigParserTest, MalformedVaultUriFailsBeforeTokenPrompt) {
    string json = R"({
      "atomdb": { "type": "redismongodb", "password": "vault://test" },
      "loaders": {},
      "agents": {},
      "vault": { "type": "openbao", "endpoint": "http://localhost:40010" }
    })";
    try {
        JsonConfigParser::load_from_string(json);
        FAIL() << "expected load_from_string to throw";
    } catch (const runtime_error& e) {
        string msg = e.what();
        EXPECT_NE(msg.find("vault://<mount>/<secret-path>"), string::npos) << msg;
        EXPECT_EQ(msg.find("stdin is not a TTY"), string::npos) << msg;
    }
}

TEST(ConfigParserTest, VaultRefsWithoutTerminalTokenFail) {
    string with_ref = R"({
      "atomdb": {
        "type": "redismongodb",
        "mongodb": { "password": "vault://test/dbs/pass" }
      },
      "loaders": {},
      "agents": {},
      "vault": { "type": "openbao", "endpoint": "localhost:40010" }
    })";
    EXPECT_THROW(JsonConfigParser::load_from_string(with_ref), runtime_error);
}
