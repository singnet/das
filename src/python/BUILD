load("@rules_python//python:defs.bzl", "py_library")
load("@rules_python//python:packaging.bzl", "py_wheel")
load("@rules_python//python:pip.bzl", "compile_pip_requirements")

package(default_visibility = ["//visibility:public"])

compile_pip_requirements(
    name = "requirements",
    src = "requirements.in",
    generate_hashes = False,
    requirements_txt = "requirements_lock.txt",
    visibility = ["//visibility:public"],
)

py_library(
    name = "hyperon_das_atomdb",
    srcs = [
        "//hyperon_das_atomdb:py_files",
        "//hyperon_das_atomdb/adapters:py_files",
        "//hyperon_das_atomdb/utils:py_files",
    ],
    deps = [
        "//hyperon_das_atomdb",
    ],
)

py_wheel(
    name = "hyperon_das_atomdb_wheel",
    distribution = "hyperon_das_atomdb",
    python_tag = "py3",
    requires_file = ":requirements_lock.txt",
    stamp = 1,
    version = "0.0.1",
    deps = [
        ":hyperon_das_atomdb",
    ],
)
