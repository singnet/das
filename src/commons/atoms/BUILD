load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "atoms_lib",
    srcs = [
    ],
    hdrs = [
    ],
    includes = ["."],
    deps = [
        ":atom",
        ":handle_decoder",
        ":link",
        ":link_schema",
        ":merger",
        ":metta_parser_actions",
        ":node",
        ":untyped_variable",
        ":wildcard",
    ],
)

cc_library(
    name = "handle_decoder",
    srcs = [],
    hdrs = ["HandleDecoder.h"],
    includes = ["."],
    deps = [":atom"],
)

cc_library(
    name = "merger",
    srcs = [],
    hdrs = ["Merger.h"],
    includes = ["."],
    deps = [":atom"],
)

cc_library(
    name = "atom",
    srcs = ["Atom.cc"],
    hdrs = ["Atom.h"],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
        "//commons:handle_trie",
    ],
)

cc_library(
    name = "link",
    srcs = ["Link.cc"],
    hdrs = ["Link.h"],
    includes = ["."],
    deps = [
        ":atom",
        ":handle_decoder",
        "//metta:metta_mapping",
    ],
)

cc_library(
    name = "node",
    srcs = ["Node.cc"],
    hdrs = ["Node.h"],
    includes = ["."],
    deps = [
        ":atom",
        "//metta:metta_mapping",
    ],
)

cc_library(
    name = "wildcard",
    srcs = ["Wildcard.cc"],
    hdrs = ["Wildcard.h"],
    includes = ["."],
    deps = [
        ":atom",
    ],
)

cc_library(
    name = "untyped_variable",
    srcs = ["UntypedVariable.cc"],
    hdrs = ["UntypedVariable.h"],
    includes = ["."],
    deps = [
        ":wildcard",
    ],
)

cc_library(
    name = "link_schema",
    srcs = ["LinkSchema.cc"],
    hdrs = ["LinkSchema.h"],
    includes = ["."],
    deps = [
        ":link",
        ":node",
        ":untyped_variable",
    ],
)

cc_library(
    name = "metta_parser_actions",
    srcs = ["MettaParserActions.cc"],
    hdrs = ["MettaParserActions.h"],
    includes = ["."],
    deps = [
        ":link",
        ":node",
        ":untyped_variable",
        "//metta:parser_actions",
    ],
)
