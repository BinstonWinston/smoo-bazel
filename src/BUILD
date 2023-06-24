load("@devkitpro//:tools.bzl", "dkp_cc_library", "dkp_header_only_cc_library", "dkp_nso", "dkp_ips_patch", "dkp_atmosphere_package")

cc_library(
    name = "smoo_lib",
    srcs = glob([
        "**/*.c",
        "**/*.cpp",
        "**/*.s",
    ]),
    hdrs = glob([
        "**/*.h",
        "**/*.hpp",
    ]),
    includes = ["."],
    copts = [
        "-I./src",
        "-I./src/sead",
    ],
    defines = [
        "EMU=0",
    ],
    deps = [
        "//proto:test_packet_cc_proto",
        "@devkitpro_sys//:libnx",
    ],
)

cc_library(
    name = "smoo_emu_lib",
    srcs = glob([
        "**/*.c",
        "**/*.cpp",
        "**/*.s",
    ]),
    hdrs = glob([
        "**/*.h",
        "**/*.hpp",
    ]),
    includes = ["."],
    copts = [
        "-I./src",
        "-I./src/sead",
    ],
    defines = [
        "EMU=1",
    ],
    deps = [
        "//proto:test_packet_cc_proto",
        "@devkitpro_sys//:libnx",
    ],
)

cc_shared_library(
    name = "smoo",
    deps = [":smoo_lib"],
)

cc_shared_library(
    name = "smoo_map",
    deps = [":smoo_lib"],
    user_link_flags = [
        "--special-dkp-output-map-file-instead-of-shared-lib",
    ],
)

cc_shared_library(
    name = "smoo_emu",
    deps = [":smoo_emu_lib"],
)

cc_shared_library(
    name = "smoo_emu_map",
    deps = [":smoo_emu_lib"],
    user_link_flags = [
        "--special-dkp-output-map-file-instead-of-shared-lib",
    ],
)

dkp_nso(
    name = "smoo_nso",
    target = ":smoo",
)

dkp_nso(
    name = "smoo_emu_nso",
    target = ":smoo_emu",
)

dkp_ips_patch(
    name = "smoo_ips",
    linker_map = ":smoo_map",
    version = "100",
    build_id = "3CA12DFAAF9C82DA064D1698DF79CDA1",
)

dkp_ips_patch(
    name = "smoo_emu_ips",
    linker_map = ":smoo_emu_map",
    version = "100",
    build_id = "3CA12DFAAF9C82DA064D1698DF79CDA1",
)

dkp_atmosphere_package(
    name = "smoo_switch",
    nso = ":smoo_nso",
    ips_patch = ":smoo_ips",
    title_id = "0100000000010000",
    romfs = "//:smoo_romfs",
)

dkp_atmosphere_package(
    name = "smoo_ryujinx",
    nso = ":smoo_emu_nso",
    ips_patch = ":smoo_ips",
    title_id = "0100000000010000",
    romfs = ":smoo_romfs",
)

# dkp_yuzu_package(
#     name = "smoo_yuzu",
#     nso = ":smoo_emu_nso",
#     romfs = ":smoo_romfs",
# )