load("@devkitpro//:tools.bzl", "dkp_header_only_cc_library")

dkp_header_only_cc_library(
    name = "al",
    srcs = glob([
        "**/*.h",
        "**/*.hpp",
    ]),
    deps = [
        "//src/sead:sead",
        "//src/game:system",
    ],
    visibility = ["//visibility:public"],
)