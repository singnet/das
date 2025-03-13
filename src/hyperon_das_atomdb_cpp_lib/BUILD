package(default_visibility = ["//visibility:public"])

cc_library(
    name = "hyperon_das_atomdb_cpp_lib",
    copts = [
        "-std=c++20",
        "-fPIC",
        "-Ofast",
    ],
    includes = ["."],
    linkstatic = True,
    deps = [
      ":constants",
      ":database",
      ":document_types",
      ":exceptions",
      ":expression_hasher",
      ":patterns",
      ":ram_only",
      ":type_aliases",
    ],
)


cc_library(
  name = "constants",
  srcs = ["constants.cc"],
  hdrs = ["constants.h"],
  deps = [
    ":expression_hasher",
    ":type_aliases",
  ],
)


cc_library(
  name = "database",
  srcs = ["database.cc"],
  hdrs = ["database.h"],
  deps = [
    ":constants",
    ":document_types",
    ":exceptions",
    ":expression_hasher",
    ":type_aliases",
  ],
)


cc_library(
  name = "document_types",
  hdrs = ["document_types.h"],
  deps = [
    ":type_aliases",
  ],
)


cc_library(
  name = "exceptions",
  hdrs = ["exceptions.h"],
)


cc_library(
  name = "expression_hasher",
  srcs = ["expression_hasher.cc"],
  hdrs = ["expression_hasher.h"],
  deps = [
    ":type_aliases",
    "@mbedtls//:mbedtls",
  ],
)


cc_library(
  name = "patterns",
  hdrs = ["patterns.h"],
  deps = [
    ":constants",
    ":expression_hasher",
  ],
)


cc_library(
  name = "ram_only",
  srcs = ["ram_only.cc"],
  hdrs = ["ram_only.h"],
  deps = [
    ":exceptions",
    ":expression_hasher",
    ":patterns",
    ":database",
    ":document_types",
    "type_aliases",
  ],
)


cc_library(
  name = "type_aliases",
  hdrs = ["type_aliases.h"],
)
