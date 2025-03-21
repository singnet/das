load("@pypi//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_library")

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "py_files",
    srcs = glob(["*.py"]),
)

py_library(
    name = "pattern_matcher",
    srcs = [":py_files"],
    deps = [
        "//hyperon_das_atomdb",
    ],
)
