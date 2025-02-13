load("@rules_python//python:defs.bzl", _py_test = "py_test")
load("@pypi_dev//:requirements.bzl", "requirement")

def py_test(name, srcs, deps=[], args=[], **kwargs):
    _py_test(
        name = name,
        main = "//tools/bazel/pytest:main.py",
        srcs = srcs + ["//tools/bazel/pytest:main.py"],
        deps = deps + [requirement("pytest")],
        args = args + ["--import-mode=importlib"] + ["$(location :%s)" % s for s in srcs],
        legacy_create_init = False,
        **kwargs,
    )

