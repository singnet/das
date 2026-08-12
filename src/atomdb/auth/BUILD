load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "authorization_types",
    srcs = [
        "AuthorizationEntry.cc",
        "AuthorizationManifest.cc",
    ],
    hdrs = [
        "AuthorizationEntry.h",
        "AuthorizationManifest.h",
    ],
    includes = ["."],
    deps = [
        "//commons/atoms:atoms_lib",
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
    name = "mongo_authorization_persistence",
    srcs = ["MongoAuthorizationPersistence.cc"],
    hdrs = ["MongoAuthorizationPersistence.h"],
    includes = ["."],
    deps = [
        ":authorization_persistence",
        ":authorization_types",
        "//commons:commons_lib",
    ],
)

cc_library(
    name = "authorization_management",
    srcs = ["AuthorizationManagement.cc"],
    hdrs = ["AuthorizationManagement.h"],
    includes = ["."],
    deps = [
        ":authorization_persistence",
        ":authorization_types",
        "//atomdb",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
        "@nlohmann_json//:json",
    ],
)
