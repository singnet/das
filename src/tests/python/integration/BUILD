load("@pypi//:requirements.bzl", "requirement")
load("@pypi_dev//:requirements.bzl", dev_requirement = "requirement")
load("@rules_python//python:defs.bzl", "py_library")
load("//tools/bazel/pytest:defs.bzl", "py_test")

package(default_visibility = ["//visibility:public"])

py_library(
    name = "helpers",
    srcs = ["helpers.py"],
    data = [
        "//tests/python/integration/docker:compose_das_alpine",
        "//tests/python/integration/scripts:sh_files",
    ],
    deps = [
        "//hyperon_das",
        "//hyperon_das:client",
        "//hyperon_das:das",
        requirement("hyperon_das_atomdb"),
    ],
)

py_library(
    name = "conftest",
    srcs = ["conftest.py"],
    deps = [
        ":helpers",
    ],
)

py_test(
    name = "test_attention_broker_gateway",
    size = "large",
    srcs = ["test_attention_broker_gateway.py"],
    deps = [
        "//hyperon_das/cache:attention_broker_gateway",
        "//hyperon_das/grpc:attention_broker_pb2_grpc",
        "//hyperon_das/grpc:common_pb2",
        requirement("grpcio"),
    ],
)

py_test(
    name = "test_client",
    # TODO: Fix #263 https://github.com/singnet/das/issues/263 
    # skiping due to infraestructure issues related to this test
    tags = ["skip"],
    size = "large",
    srcs = ["test_client.py"],
    deps = [
        ":conftest",
        ":helpers",
        "//hyperon_das:client",
        "//hyperon_das:link_filters",
        requirement("hyperon_das_atomdb"),
    ],
)

py_test(
    name = "test_das_query_api",
    # TODO: Fix #263 https://github.com/singnet/das/issues/263 
    # skiping due to infraestructure issues related to this test
    tags = ["skip"],
    size = "large",
    srcs = ["test_das_query_api.py"],
    deps = [
        ":conftest",
        ":helpers",
        "//hyperon_das",
    ],
)

py_test(
    name = "test_iterators",
    # TODO: Fix #263 https://github.com/singnet/das/issues/263 
    # skiping due to infraestructure issues related to this test
    tags = ["skip"],
    size = "large",
    srcs = ["test_iterators.py"],
    deps = [
        ":conftest",
        ":helpers",
        "//hyperon_das",
        "//hyperon_das/cache",
        "//hyperon_das/cache:iterators",
    ],
)

py_test(
    name = "test_local_das",
    # TODO: Fix #263 https://github.com/singnet/das/issues/263 
    # skiping due to infraestructure issues related to this test
    tags = ["skip"],
    size = "large",
    srcs = ["test_local_das.py"],
    deps = [
        ":conftest",
        ":helpers",
        "//hyperon_das",
        "//tests/python:helpers",
        requirement("hyperon_das_atomdb"),
    ],
)

py_test(
    name = "test_metta_api",
    size = "large",
    srcs = ["test_metta_api.py"],
    deps = [
        "//hyperon_das",
        requirement("hyperon_das_atomdb"),
    ],
)

py_test(
    name = "test_remote_das",
    # TODO: Fix #263 https://github.com/singnet/das/issues/263 
    # skiping due to infraestructure issues related to this test
    tags = ["skip"],
    size = "large",
    srcs = ["test_remote_das.py"],
    deps = [
        ":conftest",
        ":helpers",
        "//hyperon_das",
        "//hyperon_das:exceptions",
        "//hyperon_das:link_filters",
        "//hyperon_das:traverse_engines",
        requirement("hyperon_das_atomdb"),
    ],
)

py_test(
    name = "test_traverse_engine",
    size = "large",
    srcs = ["test_traverse_engine.py"],
    deps = [
        ":conftest",
        ":helpers",
        "//hyperon_das",
        "//hyperon_das:das",
    ],
)
