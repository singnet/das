load("@pypi//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_library")

package(default_visibility = ["//visibility:public"])

py_library(
    name = "pattern_matcher",
    srcs = ["pattern_matcher.py"],
    deps = [
        requirement("hyperon_das_atomdb"),
    ],
)
