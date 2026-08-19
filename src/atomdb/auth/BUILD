load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "auth_lib",
    includes = ["."],
    deps = [
        ":authorization_management",
        ":authorization_manifest",
        ":mongo_authorization_persistence",
    ],
)

cc_library(
    name = "authorization_persistence",
    hdrs = ["AuthorizationPersistence.h"],
    includes = ["."],
    deps = [
        "//atomdb:atomdb_api_types",
    ],
)

cc_library(
    name = "mongo_authorization_persistence",
    srcs = ["MongoAuthorizationPersistence.cc"],
    hdrs = ["MongoAuthorizationPersistence.h"],
    includes = ["."],
    deps = [
        ":authorization_persistence",
        "//atomdb:atomdb_api_types",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
        "//hasher:hasher_lib",
        "@nlohmann_json//:json",
    ],
)

cc_library(
    name = "authorization_management",
    srcs = ["AuthorizationManagement.cc"],
    hdrs = ["AuthorizationManagement.h"],
    includes = ["."],
    deps = [
        ":authorization_manifest",
        ":mongo_authorization_persistence",
        "//atomdb",
        "//atomdb:atomdb_api_types",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
    ],
)

cc_library(
    name = "authorization_manifest",
    srcs = ["AuthorizationManifest.cc"],
    hdrs = ["AuthorizationManifest.h"],
    includes = ["."],
    deps = [
        "//atomdb:atomdb_api_types",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
    ],
)
