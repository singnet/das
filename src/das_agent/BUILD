load("@rules_python//python:defs.bzl", "py_library")


package(default_visibility = ["//visibility:public"])

filegroup(
    name = "py_files",
    srcs = glob(["*.py"]),
)

filegroup(
    name = "py_all_files",
    srcs = glob(["**/*.py"]),
)

py_binary(
    name = "main",
    srcs = ["main.py"],
    deps = [
      ":das_agent_node",
      "//das_agent/tokenizers:dict_query_tokenizer",
      "//hyperon_das_atomdb:hyperon_das_atomdb",
      
    ],
)

py_library(
    name = "das_agent_node",
    srcs = ["das_agent_node.py"],
    deps = [
      "//hyperon_das_node:hyperon_das_node",
    ],
)