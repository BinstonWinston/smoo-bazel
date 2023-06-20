load("@devkitpro//:tools.bzl", "dkp_header_only_cc_library")

dkp_header_only_cc_library(
    name = "game",
    srcs = glob([
        "**/*.h",
        "**/*.hpp",
    ]),
    deps = [
        "//src/al:al",
    ],
    visibility = ["//visibility:public"],
)