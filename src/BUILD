load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_shared_library")
load("@rules_proto_grpc_cpp//:defs.bzl", "cc_grpc_library", "cc_proto_library", "cpp_grpc_library")
load("@rules_pkg//pkg:files.bzl", "pkg_files", "pkg_dirs")
load("@rules_pkg//pkg:deb.bzl", "pkg_deb")
load("@rules_pkg//pkg:rpm.bzl", "pkg_rpm")

package(default_visibility = ["//visibility:public"])

exports_files(
    [
        ".ruff.toml",
        ".clang-format",
    ],
    visibility = ["//visibility:public"],
)

alias(
    name = "format.check",
    actual = "//tools/format:format.check",
)

alias(
    name = "format",
    actual = "//tools/format",
)

cc_shared_library(
    name = "das",
    additional_linker_inputs = [
    ],
    dynamic_deps = [
    ],
    shared_lib_name = "hyperon_das.so",
    user_link_flags = [
        "-L/usr/local/lib",
        "-lhiredis_cluster",
        "-lhiredis",
        "-lmongocxx",
        "-lbsoncxx",
    ],
    deps = [
        "//agents:agents_lib",
        "//agents/evolution:evolution_lib",
        "//agents/inference_agent:inference_agent_lib",
        "//agents/link_creation_agent:link_creation_agent_lib",
        "//agents/query_engine:query_engine_lib",
        "//atomdb:atomdb_lib",
        "//attention_broker:attention_broker_lib",
        "//commons:commons_lib",
        "//distributed_algorithm_node:distributed_algorithm_node_lib",
        "//hasher:hasher_lib",
        "//metta:metta_lib",
        "//service_bus:service_bus_lib",
    ],
)

cc_binary(
    name = "attention_broker_service",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//commons:commons_lib",
        "//main:attention_broker_main_lib",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@grpc//:grpc++",
        "@grpc//:grpc++_reflection",
        "@mbedtls",
    ],
)

cc_binary(
    name = "attention_broker_client",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkopts = [
        "-L/usr/local/lib",
        "-lhiredis_cluster",
        "-lhiredis",
        "-lmongocxx",
        "-lbsoncxx",
    ],
    linkstatic = 1,
    deps = [
        "//commons:commons_lib",
        "//main:attention_broker_client_main_lib",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@grpc//:grpc++",
        "@grpc//:grpc++_reflection",
        "@mbedtls",
    ],
)

cc_binary(
    name = "tests_db_loader",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkopts = [
        "-L/usr/local/lib",
        "-lhiredis_cluster",
        "-lhiredis",
        "-lmongocxx",
        "-lbsoncxx",
    ],
    linkstatic = 1,
    deps = [
        "//atomdb:atomdb_lib",
        "//commons:commons_lib",
        "//tests/main:tests_db_loader_main_lib",
    ],
)

cc_binary(
    name = "link_creation_engine",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//commons:commons_lib",
        "//tests/main:link_creation_engine_main_lib",
        "@mbedtls",
    ],
)

cc_binary(
    name = "word_query",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//commons:commons_lib",
        "//tests/main:word_query_main_lib",
        "@mbedtls",
    ],
)

cc_binary(
    name = "word_query_evolution",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//commons:commons_lib",
        "//tests/main:word_query_evolution_main_lib",
        "@mbedtls",
    ],
)

cc_binary(
    name = "implication_query_evolution",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//commons:commons_lib",
        "//tests/main:implication_query_evolution_main_lib",
        "@mbedtls",
    ],
)

cc_binary(
    name = "busnode",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//main:bus_node_lib",
    ],
)

cc_binary(
    name = "busclient",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//main:bus_client_lib",
    ],
)

# RPM and DEB Packaging setup



pkg_tar(
    name = "binaries_tar",
    srcs = [
        "//:attention_broker_service",
        "//:attention_broker_client",
        "//:busnode",
        "//:busclient",
        "//:word_query",
        "//:word_query_evolution",
        "//:implication_query_evolution",
        "//:tests_db_loader",
    ],
    package_dir = "/usr/bin",
)

pkg_tar(
    name = "libs_tar",
    srcs = [
        "//:das",
    ],
    package_dir = "/usr/lib",
)

pkg_tar(
    name = "external_deps",
    srcs = glob(["ext_libs/*.so*"], allow_empty = True),
    package_dir = "/usr/lib",
)

pkg_tar(
    name = "das_tar",
    deps = [
        ":binaries_tar",
        ":libs_tar",
        ":external_deps"
    ],
)

pkg_deb(
    name = "das_deb_package",
    package = "das",
    architecture = "amd64",
    version = "1.0.1",
    data = ":das_tar",
    description = "Distributed AtomSpace Binaries and Libraries",
    maintainer = "Hyperon DAS Team",
)

pkg_rpm(
    name = "das_rpm_package",
    package_name = "das",
    architecture = "x86_64",
    version = "1.0.1",
    srcs = [":das_tar"],
    summary = "Distributed AtomSpace Binaries and Libraries",
    description = "Distributed AtomSpace Binaries and Libraries and system dependencies",
    license = "Apache-2.0",
    rpmbuild_path = "/usr/bin/rpmbuild",
    target_compatible_with = select({
        "@platforms//os:linux": [],
        "//conditions:default": ["@platforms//:incompatible"],
    }),
)