load("@devkitpro//:tools.bzl", "dkp_cc_library")

dkp_cc_library(
    name = "sead",
    srcs = glob([
        "**/*.cpp",
        "**/*.c",
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
    deps = [
        "//src:types_lib",
        "//src/nn:nn",
        "@devkitpro_sys//:libnx"
    ],
    visibility = ["//visibility:public"],
)