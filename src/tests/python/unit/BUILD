load("@pypi//:requirements.bzl", "requirement")
load("@pypi_dev//:requirements.bzl", dev_requirement = "requirement")
load("@rules_python//python:defs.bzl", "py_library")
load("//tools/bazel/pytest:defs.bzl", "py_test")

package(default_visibility = ["//visibility:public"])

py_library(
    name = "fixtures",
    srcs = ["fixtures.py"],
    deps = [
        "//hyperon_das_atomdb/adapters",
        dev_requirement("mongomock"),
    ],
)

py_test(
    name = "test_database_public_methods",
    srcs = ["test_database_public_methods.py"],
    data = ["//tests/python/unit/adapters/data:json_data"],
    deps = [
        ":fixtures",
        "//hyperon_das_atomdb:database",
        "//tests/python:helpers",
    ],
)

py_test(
    name = "test_database_private_methods",
    srcs = ["test_database_private_methods.py"],
    deps = [
        ":fixtures",
        "//hyperon_das_atomdb:database",
        "//hyperon_das_atomdb:exceptions",
        "//tests/python:helpers",
    ],
)

py_library(
    name = "helpers",
    srcs = ["helpers.py"],
    deps = [
        "//hyperon_das",
    ],
)

py_library(
    name = "mock",
    srcs = ["mock.py"],
    deps = [
        "//hyperon_das",
        "//hyperon_das:das",
        "//hyperon_das/cache:cache_controller",
        "//hyperon_das_atomdb",
    ],
)

py_test(
    name = "test_attention_broker_gateway",
    srcs = ["test_attention_broker_gateway.py"],
    deps = [
        "//hyperon_das/cache:attention_broker_gateway",
    ],
)

py_test(
    name = "test_cache_controller",
    srcs = ["test_cache_controller.py"],
    deps = [
        "//hyperon_das:context",
        "//hyperon_das:utils",
        "//hyperon_das/cache:attention_broker_gateway",
        "//hyperon_das/cache:cache_controller",
        "//hyperon_das_atomdb",
    ],
)

py_test(
    name = "test_cache",
    srcs = ["test_cache.py"],
    deps = [
        "//hyperon_das:utils",
        "//hyperon_das/cache:iterators",
    ],
)

py_test(
    name = "test_client",
    srcs = ["test_client.py"],
    deps = [
        "//hyperon_das:client",
        "//hyperon_das:exceptions",
        "//hyperon_das:link_filters",
        "//hyperon_das:utils",
        requirement("requests"),
    ],
)

py_test(
    name = "test_context",
    srcs = ["test_context.py"],
    deps = [
        "//hyperon_das:context",
        "//hyperon_das_atomdb",
    ],
)

py_test(
    name = "test_das",
    srcs = ["test_das.py"],
    deps = [
        ":mock",
        "//hyperon_das:context",
        "//hyperon_das:das",
        "//hyperon_das:traverse_engines",
        "//hyperon_das_atomdb",
    ],
)

py_test(
    name = "test_decorators",
    srcs = ["test_decorators.py"],
    deps = [
        ":mock",
        "//hyperon_das:decorators",
        "//hyperon_das:exceptions",
    ],
)

py_test(
    name = "test_dict_query_tokenizer",
    srcs = ["test_dict_query_tokenizer.py"],
    deps = [
        "//hyperon_das/tokenizers:dict_query_tokenizer",
    ],
)

py_test(
    name = "test_elements",
    srcs = ["test_elements.py"],
    deps = [
        "//hyperon_das/tokenizers:elements",
    ],
)

py_test(
    name = "test_evolution",
    srcs = ["test_evolution.py"],
    deps = [
        "//evolution:fitness_functions",
        "//evolution:optimizer",
        "//evolution:selection_methods",
        "//evolution:utils",
        "//hyperon_das",
        "//hyperon_das_node",
        "//hyperon_das_query_engine",
        "//hyperon_das_atomdb",
    ],
)

py_test(
    name = "test_queries",
    srcs = ["test_queries.py"],
    deps = [
        "//hyperon_das:constants",
        "//hyperon_das:das",
    ],
)

py_test(
    name = "test_traverse_engine",
    srcs = ["test_traverse_engine.py"],
    deps = [
        "//hyperon_das:constants",
        "//hyperon_das:das",
        "//tests/python:helpers",
        "//hyperon_das_atomdb",
    ],
)

py_test(
    name = "test_utils",
    srcs = ["test_utils.py"],
    deps = [
        "//hyperon_das:exceptions",
        "//hyperon_das:utils",
    ],
)

py_test(
    name = "test_star_node",
    tags = ["skip"],
    size = "small",
    srcs = ["test_star_node.py"],
    deps = [
        "//hyperon_das_node:hyperon_das_node",
    ],
)

py_test(
    name = "test_das_node",
    size = "small",
    srcs = ["test_das_node.py"],
    deps = [
        "//hyperon_das_query_engine:hyperon_das_query_engine",
    ],
)

