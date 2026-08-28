load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "auth_lib",
    includes = ["."],
    deps = [
        ":authorization_manager",
        ":authorization_manifest",
        ":mongodb_authorization_persistence",
    ],
)

cc_library(
    name = "authorization_persistence",
    hdrs = ["AuthorizationPersistence.h"],
    includes = ["."],
    deps = [
        ":authorization_types",
    ],
)

cc_library(
    name = "mongodb_authorization_persistence",
    srcs = ["MongodbAuthorizationPersistence.cc"],
    hdrs = ["MongodbAuthorizationPersistence.h"],
    includes = ["."],
    deps = [
        ":authorization_persistence",
        ":authorization_types",
        "//atomdb/redis_mongodb:redis_mongodb_api_types",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
        "//hasher:hasher_lib",
    ],
)

cc_library(
    name = "authorization_manager",
    srcs = ["AuthorizationManager.cc"],
    hdrs = ["AuthorizationManager.h"],
    includes = ["."],
    deps = [
        ":authorization_persistence",
        ":authorization_types",
        ":mongodb_authorization_persistence",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
    ],
)

cc_library(
    name = "authorization_types",
    srcs = ["AuthorizationTypes.cc"],
    hdrs = ["AuthorizationTypes.h"],
    includes = ["."],
    deps = [
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
        ":authorization_types",
        "//atomdb:atomdb_api_types",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
    ],
)
