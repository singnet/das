load("@rules_pkg//pkg:deb.bzl", "pkg_deb")
load("@rules_pkg//pkg:mappings.bzl", "pkg_filegroup", "pkg_files")
load("@rules_pkg//pkg:rpm.bzl", "pkg_rpm")
load("@rules_pkg//pkg:tar.bzl", "pkg_tar")

# RPM Packaging Setup

pkg_files(
    name = "binaries_files",
    srcs = [
        "//:attention_broker_client",
        "//:attention_broker_service",
        "//:busclient",
        "//:busnode",
        "//:database_adapter",
        "//:implication_query_evolution",
        "//:tests_db_loader",
        "//:word_query",
        "//:word_query_evolution",
    ],
    prefix = "/usr/bin",
)

pkg_files(
    name = "libs_files",
    srcs = [
        "//:das",
    ],
    prefix = "/usr/lib",
)

pkg_files(
    name = "external_deps_files",
    srcs = glob(
        ["ext_libs/*.so*"],
        allow_empty = True,
    ),
    prefix = "/usr/lib",
)

pkg_filegroup(
    name = "das_pkg_files",
    srcs = [
        ":binaries_files",
        ":external_deps_files",
        ":libs_files",
    ],
)

pkg_rpm(
    name = "das_rpm_package",
    package_name = "das",
    srcs = [
        ":binaries_files",
        ":external_deps_files",
        ":libs_files",
    ],
    architecture = "x86_64",
    description = "Distributed AtomSpace Binaries and Libraries and system dependencies",
    license = "Apache-2.0",
    release = "1",
    rpmbuild_path = "/usr/bin/rpmbuild",
    summary = "Distributed AtomSpace Binaries and Libraries",
    target_compatible_with = select({
        "@platforms//os:linux": [],
        "//conditions:default": ["@platforms//:incompatible"],
    }),
    version = "1.0.3",
)

# DEB Packaging Setup

pkg_tar(
    name = "binaries_tar",
    srcs = [
        "//:attention_broker_client",
        "//:attention_broker_service",
        "//:busclient",
        "//:busnode",
        "//:database_adapter",
        "//:implication_query_evolution",
        "//:tests_db_loader",
        "//:word_query",
        "//:word_query_evolution",
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
    srcs = glob(
        ["ext_libs/*.so*"],
        allow_empty = False,
    ),
    package_dir = "/usr/lib",
)

pkg_tar(
    name = "das_tar",
    deps = [
        ":binaries_tar",
        ":external_deps",
        ":libs_tar",
    ],
)

pkg_deb(
    name = "das_deb_package",
    architecture = "amd64",
    data = "das_tar",
    description = "Distributed AtomSpace Binaries and Libraries",
    maintainer = "Hyperon DAS Team",
    package = "das",
    version = "1.0.3",
)
