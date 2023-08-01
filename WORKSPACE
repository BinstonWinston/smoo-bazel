load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "rules_proto",
    sha256 = "dc3fb206a2cb3441b485eb1e423165b231235a1ea9b031b4433cf7bc1fa460dd",
    strip_prefix = "rules_proto-5.3.0-21.7",
    urls = [
        "https://github.com/bazelbuild/rules_proto/archive/refs/tags/5.3.0-21.7.tar.gz",
    ],
)
load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")
rules_proto_dependencies()
rules_proto_toolchains()

local_repository(
    name = "devkitpro",
    path = "./devkitpro-bazel",
)

new_local_repository(
    name = "devkitpro_sys",
    path = "/opt/devkitpro",
    build_file = "./devkitpro-bazel/devkitpro_sys.BUILD"
)

register_execution_platforms(
    #    The first toolchain is present to cause targets which were not defined
    # to use the custom toolchain to select an execution platform with
    # an undefined value for constraint setting nonstandard_toolchain. The
    # undefined value will then mismatch the value for nonstandard_toolchain of
    # @devkitpro//:devkitpro_nx_toolchain (which is simple_cpp_toolchain).
    #    The dependency of a target on the non-standard toolchain is specified
    # by having the exec_compatible_with attribute of the target include
    # @devkitpro//devkitpro_toolchain:simple_cpp_toolchain. This causes the first
    # execution platform to be rejected during execution platform selection.
    "@local_config_platform//:host",
    "@devkitpro//:devkitpro_nx_platform"
)

register_toolchains(
    "@devkitpro//:devkitpro_nx_toolchain"
)

http_archive(
    name = "rules_dotnet",
    sha256 = "77575d68c609d98b92f3df8db79944e7b60c035766e1c233349aeb1659c86ff9",
    strip_prefix = "rules_dotnet-0.8.12",
    url = "https://github.com/bazelbuild/rules_dotnet/releases/download/v0.8.12/rules_dotnet-v0.8.12.tar.gz",
)

load(
    "@rules_dotnet//dotnet:repositories.bzl",
    "dotnet_register_toolchains",
    "rules_dotnet_dependencies",
)

rules_dotnet_dependencies()

# Here you can specify the version of the .NET SDK to use.
dotnet_register_toolchains("dotnet", "7.0.101")

load("@rules_dotnet//dotnet:rules_dotnet_nuget_packages.bzl", "rules_dotnet_nuget_packages")

rules_dotnet_nuget_packages()