# Toolchain target definition
load(":devkitpro_toolchain.bzl", "cc_toolchain_config_info_generator")

cc_toolchain_config_info_generator(name = "devkitpro_nx_config")

filegroup(name = "empty")

cc_toolchain(
    name                    = "devkitpro_nx",
    all_files               = ":empty",
    # ar_files (optional)
    # as_files (optional)
    # compiler (deprecated, optional)
    compiler_files          = ":empty",
    # compiler_files_without_includes (optional)
    # coverage_files (optional)
    # cpu (deprecated, optional)
    dwp_files               = ":empty",
    # dynamic_runtime_lib (optional)
    # libc_top (optional)
    linker_files            = ":empty",
    # module_map (optional)
    objcopy_files           = ":empty",
    # static_runtime_lib (optional)
    strip_files             = ":empty",
    supports_header_parsing = False,
    supports_param_files    = False,
    toolchain_config        = ":devkitpro_nx_config",
    # toolchain_identifier (optional)
)

# Information which describes the cc_toolchain target to the general toolchain
# apparatus of Bazel.
toolchain(
    name                   = "devkitpro_nx_toolchain",
    exec_compatible_with   = [
        "//devkitpro_toolchain:simple_cpp_toolchain",
        "@platforms//os:linux",
        "@platforms//cpu:aarch64"
    ],
    target_compatible_with = [
        "@platforms//os:linux",
        "@platforms//cpu:aarch64"
    ],
    # target_settings        = [],
    toolchain              = ":devkitpro_nx",
    # The toolchain_type attribute is an identifier that is used by a rule,
    # such as cc_binary, to indicate that the rule uses the identified type of
    # toolchain. Rules can use more than one toolchain_type. C++ rules use a
    # single toolchain_type. The name of a toolchain_type is, by convention,
    # always "toolchain_type".
    toolchain_type         = "@rules_cc//cc:toolchain_type",
    visibility             = ["//visibility:public"]
)

# A platform target definition for targets which use the non-standard
# toolchain.
platform(
    name                 = "devkitpro_nx_platform",
    constraint_values    = [
        "//devkitpro_toolchain:simple_cpp_toolchain",
        "@platforms//cpu:aarch64",
        "@platforms//os:linux"
    ]
)