load("@python_deps//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_library")

package(default_visibility = ["//visibility:public"])

py_library(
    name = "expression_hasher",
    srcs = ["expression_hasher.py"],
    deps = [],
)

py_library(
    name = "settings",
    srcs = ["settings.py"],
    deps = [
        requirement("python-dotenv"),
    ],
)
