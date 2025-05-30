load("@rules_python//python:pip.bzl", "compile_pip_requirements")

package(default_visibility = ["//visibility:public"])

compile_pip_requirements(
    name = "requirements_python_client",
    src = "requirements_python_client.in",
    generate_hashes = False,
    requirements_txt = "requirements_python_client_lock.txt",
    visibility = ["//visibility:public"],
)
