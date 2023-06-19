load("@devkitpro//:tools.bzl", "dkp_cc_binary", "dkp_cc_library", "dkp_nacp", "dkp_nro")

dkp_cc_library(
    name = "smoo_lib",
    srcs = glob([
        "**/*.cpp",
        "**/*.s",
    ]),
    hdrs = glob([
        "**/*.h",
        "**/*.hpp",
    ]),
    deps = ["@devkitpro_sys//:libnx"],
)

dkp_cc_binary(
    name = "smoo",
    srcs = ["main.cpp"],
    deps = [
        ":smoo_lib",
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

# TODO smoo_atmosphere and smoo_yuzu packaged builds with new rules that copy romfs as well