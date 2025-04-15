load("@pypi_das//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_library")

package(default_visibility = ["//visibility:public"])

py_library(
    name = "query_engine_protocol",
    srcs = ["query_engine_protocol.py"],
    deps = [
        "//hyperon_das:context",
        "//hyperon_das:link_filters",
        "//hyperon_das:type_alias",
        "//hyperon_das:utils",
        "//hyperon_das_atomdb",
    ],
)

py_library(
    name = "local_query_engine",
    srcs = ["local_query_engine.py"],
    deps = [
        ":query_engine_protocol",
        "//hyperon_das:client",
        "//hyperon_das:context",
        "//hyperon_das:exceptions",
        "//hyperon_das:link_filters",
        "//hyperon_das:logger",
        "//hyperon_das:type_alias",
        "//hyperon_das:utils",
        "//hyperon_das/cache:cache_controller",
        "//hyperon_das/cache:iterators",
        "//hyperon_das_atomdb",
    ],
)

py_library(
    name = "remote_query_engine",
    srcs = ["remote_query_engine.py"],
    deps = [
        ":local_query_engine",
        ":query_engine_protocol",
        "//hyperon_das:client",
        "//hyperon_das:context",
        "//hyperon_das:exceptions",
        "//hyperon_das:link_filters",
        "//hyperon_das:type_alias",
        "//hyperon_das:utils",
        "//hyperon_das/cache:cache_controller",
        "//hyperon_das/cache:iterators",
        "//hyperon_das_atomdb",
    ],
)
