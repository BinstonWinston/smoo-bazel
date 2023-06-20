load("@devkitpro//:tools.bzl", "dkp_cc_library", "dkp_header_only_cc_library")

dkp_header_only_cc_library(
    name = "hostname_lib",
    srcs = ["Hostname.hpp"],
    deps = [
        "//src/sead:sead",
    ],
    visibility = ["//visibility:public"],
)

dkp_cc_library(
    name = "server_lib_no_emu",
    srcs = [
        "logger.cpp",
        "SocketBase.cpp",
        "SocketClient.cpp",
    ],
    public_hdrs = [
        "logger.hpp",
        "SocketBase.hpp",
        "SocketClient.hpp",
    ],
    includes = [
        "./src",
        "./src/sead",
    ],
    deps = [
        ":hostname_lib",
        "//src:types_lib",
        "//src:helpers_lib",
        "//src/al:al",
        "//src/sead:sead",
        "//src/nn:nn",
        "@devkitpro_sys//:libnx",
    ],
    visibility = ["//visibility:public"]
)