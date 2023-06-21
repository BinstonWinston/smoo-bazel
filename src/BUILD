load("@devkitpro//:tools.bzl", "dkp_cc_library", "dkp_header_only_cc_library", "dkp_nacp", "dkp_nro")

dkp_cc_library(
    name = "smoo",
    srcs = glob([
        "**/*.c",
        "**/*.cpp",
        "**/*.s",
    ]),
    public_hdrs = glob([
        "**/*.h",
        "**/*.hpp",
    ]),
    includes = [
        "./src",
        "./src/sead",
    ],
    emu = False,
    deps = [
        "@devkitpro_sys//:libnx",
    ],
)

dkp_cc_library(
    name = "smoo_emu",
    srcs = glob([
        "**/*.c",
        "**/*.cpp",
        "**/*.s",
    ]),
    public_hdrs = glob([
        "**/*.h",
        "**/*.hpp",
    ]),
    includes = [
        "./src",
        "./src/sead",
    ],
    emu = True,
    deps = [
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