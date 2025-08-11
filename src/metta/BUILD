load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "metta_lib",
    srcs = [],
    hdrs = [],
    includes = ["."],
    deps = [
        ":metta_lexer",
        ":metta_mapping",
        ":metta_parser",
        ":parser_actions",
    ],
)

cc_library(
    name = "metta_lexer",
    srcs = [
        "MettaLexer.cc",
        "MettaTokens.cc",
        "Token.cc",
    ],
    hdrs = [
        "MettaLexer.h",
        "MettaTokens.h",
        "Token.h",
    ],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
    ],
)

cc_library(
    name = "metta_parser",
    srcs = [
        "MettaParser.cc",
    ],
    hdrs = [
        "MettaParser.h",
    ],
    includes = ["."],
    deps = [
        ":metta_lexer",
        "//commons:commons_lib",
    ],
)

cc_library(
    name = "parser_actions",
    srcs = [
        "ParserActions.cc",
    ],
    hdrs = [
        "ParserActions.h",
    ],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
    ],
)

cc_library(
    name = "metta_mapping",
    srcs = [
        "MettaMapping.cc",
    ],
    hdrs = [
        "MettaMapping.h",
    ],
    includes = ["."],
    deps = [
    ],
)
