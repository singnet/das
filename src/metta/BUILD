load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "metta_lib",
    srcs = [],
    hdrs = [],
    includes = ["."],
    deps = [
        ":metta_lexer",
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
