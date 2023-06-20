load("@devkitpro//:tools.bzl", "dkp_cc_library", "dkp_header_only_cc_library", "dkp_nacp", "dkp_nro")

dkp_header_only_cc_library(
    name = "types_lib",
    srcs = ["types.h"],
    visibility = ["//visibility:public"]
)

dkp_cc_library(
    name = "helpers_lib",
    srcs = ["helpers.cpp"],
    public_hdrs = ["helpers.hpp"],
    includes = [
        "./src",
        "./src/sead",
    ],
    deps = [
        ":types_lib",
        "//src/sead:sead",
        "//src/al:al",
        "@devkitpro_sys//:libnx",
    ],
    visibility = ["//visibility:public"]
)

dkp_cc_library(
    name = "smoo",
    srcs = ["main.cpp"],
    public_hdrs = glob([
        "**/*.h",
        "**/*.hpp",
    ]),
    includes = [
        "./src",
        "./src/sead",
    ],
    deps = [
        "//src/sead:sead",
        "//src/server:server_lib_no_emu",
        "@devkitpro_sys//:libnx",
    ],
)

dkp_cc_library(
    name = "smoo_emu",
    srcs = ["main.cpp"],
    public_hdrs = glob([
        "**/*.h",
        "**/*.hpp",
    ]),
    includes = [
        "./src",
        "./src/sead",
    ],
    deps = [
        ":smoo_lib",
        "//src/server:server_lib_emu",
        "@devkitpro_sys//:libnx",
    ],
)

dkp_nacp(
    name = "smoo_nacp",
    nacp_name = "smoo",
    author = "IcyIceberg",
    version = "1.0.0",
    titleid = "0100000000010000",
)

dkp_nro(
    name = "smoo_nro",
    target = ":smoo",
    nacp = ":smoo_nacp",
)

dkp_nro(
    name = "smoo_emu_nro",
    target = ":smoo_emu",
    nacp = ":smoo_nacp",
)

# dkp_atmosphere_package(
#     name = "smoo_switch",
#     nro = ":smoo_nro",
#     romfs = ":smoo_romfs",
# )

# dkp_atmosphere_package(
#     name = "smoo_ryujinx",
#     nro = ":smoo_emu_nro",
#     romfs = ":smoo_romfs",
# )

# dkp_yuzu_package(
#     name = "smoo_yuzu",
#     nro = ":smoo_emu_nro",
#     romfs = ":smoo_romfs",
# )