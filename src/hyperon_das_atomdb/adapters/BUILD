load("@pypi_atomdb//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_library")

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "py_files",
    srcs = glob(["*.py"]),
)

py_library(
    name = "adapters",
    srcs = [":py_files"],
    deps = [
        ":ram_only",
        ":redis_mongo_db",
    ],
)

py_library(
    name = "ram_only",
    srcs = ["ram_only.py"],
    deps = [
        "//hyperon_das_atomdb_cpp",
    ],
)

py_library(
    name = "redis_mongo_db",
    srcs = ["redis_mongo_db.py"],
    deps = [
        "//hyperon_das_atomdb:database",
        "//hyperon_das_atomdb:exceptions",
        "//hyperon_das_atomdb:index",
        "//hyperon_das_atomdb:logger",
        "//hyperon_das_atomdb/utils:expression_hasher",
        requirement("pymongo"),
        requirement("redis"),
    ],
)
