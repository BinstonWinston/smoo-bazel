load("@devkitpro//:tools.bzl", "dkp_cc_library", "dkp_nso", "dkp_ips_patch", "dkp_atmosphere_package")

dkp_cc_library(
    name = "smoo",
    srcs = glob([
        "**/*.c",
        "**/*.cpp",
        "**/*.s",
    ]),
    hdrs = glob([
        "**/*.h",
        "**/*.hpp",
    ]),
    includes = [
        ".",
    ],
    copts = [
        "-I./src",
        "-I./src/sead",
    ],
    emu = False,
    deps = [
        "//proto/packets:packets_cc_proto",
    ],
)

dkp_cc_library(
    name = "smoo_emu",
    srcs = glob([
        "**/*.c",
        "**/*.cpp",
        "**/*.s",
    ]),
    hdrs = glob([
        "**/*.h",
        "**/*.hpp",
    ]),
    includes = [
        ".",
        "./src",
        "./src/sead",
    ],
    emu = True,
    deps = [
        "//proto/packets:packets_cc_proto",
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
    linker_map = ":smoo_linker_map",
    version = "100",
    build_id = "3CA12DFAAF9C82DA064D1698DF79CDA1",
)

dkp_ips_patch(
    name = "smoo_emu_ips",
    linker_map = ":smoo_emu_linker_map",
    version = "100",
    build_id = "3CA12DFAAF9C82DA064D1698DF79CDA1",
)

dkp_atmosphere_package(
    name = "smoo_switch",
    nso = ":smoo_nso",
    ips_patch = ":smoo_ips",
    title_id = "0100000000010000",
    romfs = "//romfs:smoo_romfs",
    build_id = "3CA12DFAAF9C82DA064D1698DF79CDA1",
)

dkp_atmosphere_package(
    name = "smoo_ryujinx",
    nso = ":smoo_emu_nso",
    ips_patch = ":smoo_ips",
    title_id = "0100000000010000",
    romfs = "//romfs:smoo_romfs",
    build_id = "3CA12DFAAF9C82DA064D1698DF79CDA1",
)

# dkp_yuzu_package(
#     name = "smoo_yuzu",
#     nso = ":smoo_emu_nso",
#     romfs = "//romfs:smoo_romfs",
# )