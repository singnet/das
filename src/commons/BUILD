load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

# Leaf helpers used by vault backends without depending on JsonConfigParser
# (avoids a cycle: commons_lib -> vault resolver -> utils).
cc_library(
    name = "utils_lib",
    srcs = ["Utils.cc"],
    hdrs = [
        "Logger.h",
        "Profiler.h",
        "Utils.h",
    ],
    includes = ["."],
)

cc_library(
    name = "openbao_config",
    srcs = ["vault/OpenBaoConfig.cc"],
    hdrs = ["vault/OpenBaoConfig.h"],
    includes = ["vault"],
    deps = [
        ":utils_lib",
        "@nlohmann_json//:json",
    ],
)

cc_library(
    name = "openbao_client",
    srcs = ["vault/OpenBaoClient.cc"],
    hdrs = ["vault/OpenBaoClient.h"],
    includes = ["vault"],
    deps = [
        ":openbao_config",
        ":utils_lib",
        "@nlohmann_json//:json",
    ],
)

cc_library(
    name = "vault_json_resolver",
    srcs = ["vault/VaultJsonResolver.cc"],
    hdrs = ["vault/VaultJsonResolver.h"],
    includes = ["vault"],
    deps = [
        ":openbao_client",
        ":openbao_config",
        ":utils_lib",
        "@nlohmann_json//:json",
    ],
)

cc_library(
    name = "commons_lib",
    srcs = [
        "Assignment.cc",
        "JsonConfig.cc",
        "JsonConfigParser.cc",
        "SharedQueue.cc",
        "StoppableThread.cc",
        "SystemParameters.cc",
        "SystemParametersSingleton.cc",
        "SystemParametersValidation.cc",
    ],
    hdrs = [
        "Assignment.h",
        "JsonConfig.h",
        "JsonConfigParser.h",
        "Logger.h",
        "MongoInitializer.h",
        "Profiler.h",
        "Properties.h",
        "RequestValidator.h",
        "Serializable.h",
        "SharedQueue.h",
        "Stoppable.h",
        "StoppableThread.h",
        "SystemParameters.h",
        "SystemParametersSingleton.h",
        "SystemParametersValidation.h",
        "ThreadPool.h",
        "ThreadSafeHashmap.h",
        "ThreadSafeHeap.h",
        "ThreadSafeQueue.h",
        "Utils.h",
    ],
    includes = ["."],
    deps = [
        ":utils_lib",
        ":vault_json_resolver",
        "//hasher:hasher_lib",
        "@nlohmann_json//:json",
    ],
)

cc_library(
    name = "handle_trie",
    srcs = ["HandleTrie.cc"],
    hdrs = ["HandleTrie.h"],
    deps = [
        ":commons_lib",
        "//hasher:hasher_lib",
    ],
)
