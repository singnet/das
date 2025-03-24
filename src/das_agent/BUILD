load("@rules_python//python:defs.bzl", "py_library")
load("@pypi//:requirements.bzl", "requirement")


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



# py_library(
#     name = "selection_methods",
#     srcs = ["selection_methods.py"],
#     deps = [],
# )

# py_library(
#     name = "utils",
#     srcs = ["utils.py"],
#     deps = [],
# )

# py_library(
#     name = "optimizer",
#     srcs = ["optimizer.py"],
#     deps = [
#         ":fitness_functions",
#         ":selection_methods",
#         ":utils",
#         "//hyperon_das:hyperon_das",
#         "//hyperon_das_atomdb:hyperon_das_atomdb",
#         "//hyperon_das_query_engine:hyperon_das_query_engine",
#         # "//evolution/das_node:das_node",
#         # "//evolution/das_node:query_answer",
#     ],
# )