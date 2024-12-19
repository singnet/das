load("@python_deps//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_library")

package(default_visibility = ["//visibility:public"])

py_library(
    name = "all_adapters",
    srcs = glob(["*.py"]),
    deps = [
        ":ram_only",
        ":redis_mongo_db",
    ],
)

py_library(
    name = "ram_only",
    srcs = ["ram_only.py"],
    deps = [
        requirement("hyperon_das_atomdb_cpp"),
    ],
)

py_library(
    name = "redis_mongo_db",
    srcs = ["redis_mongo_db.py"],
    deps = [
        "//python/hyperon_das_atomdb:database",
        "//python/hyperon_das_atomdb:exceptions",
        "//python/hyperon_das_atomdb:index",
        "//python/hyperon_das_atomdb:logger",
        "//python/hyperon_das_atomdb/utils:expression_hasher",
        requirement("pymongo"),
        requirement("redis"),
    ],
)
