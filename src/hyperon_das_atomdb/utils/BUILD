load("@pypi//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_library")

package(default_visibility = ["//visibility:public"])

py_library(
    name = "utils",
    srcs = ["expression_hasher.py", "settings.py"],
    deps = [
        ":expression_hasher",
        ":settings",
    ],
)

py_library(
    name = "expression_hasher",
    srcs = ["expression_hasher.py"],
)

py_library(
    name = "settings",
    srcs = ["settings.py"],
    deps = [
        requirement("python-dotenv"),
    ],
)
