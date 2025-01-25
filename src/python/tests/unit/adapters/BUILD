load("//tools/bazel/pytest:defs.bzl", "py_test")

package(default_visibility = ["//visibility:public"])

py_test(
    name = "test_ram_only",
    srcs = ["test_ram_only.py"],
    data = ["//tests/unit/adapters/data:json_data"],
    deps = [
        "//hyperon_das_atomdb",
        "//hyperon_das_atomdb:database",
        "//hyperon_das_atomdb:exceptions",
        "//hyperon_das_atomdb/adapters",
        "//hyperon_das_atomdb/utils:expression_hasher",
        "//tests:helpers",
        "//tests/unit:fixtures",
    ],
)

py_test(
    name = "test_ram_only_extra",
    srcs = ["test_ram_only_extra.py"],
    deps = [
        "//hyperon_das_atomdb",
        "//tests/unit:fixtures",
        "//tests/unit:test_database_public_methods",
    ],
)

py_test(
    name = "test_redis_mongo_db",
    srcs = ["test_redis_mongo_db.py"],
    data = ["//tests/unit/adapters/data:json_data"],
    deps = [
        "//hyperon_das_atomdb",
        "//hyperon_das_atomdb:database",
        "//hyperon_das_atomdb:exceptions",
        "//hyperon_das_atomdb/adapters:redis_mongo_db",
        "//hyperon_das_atomdb/utils:expression_hasher",
        "//tests:helpers",
        "//tests/unit:fixtures",
    ],
)

py_test(
    name = "test_redis_mongo_extra",
    srcs = ["test_redis_mongo_extra.py"],
    data = ["//tests/unit/adapters/data:json_data"],
    deps = [
        "//hyperon_das_atomdb/adapters:redis_mongo_db",
        "//tests:helpers",
        "//tests/unit:fixtures",
    ],
)
