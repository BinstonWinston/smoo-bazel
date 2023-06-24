load("@rules_proto//proto:defs.bzl", "proto_library")

cc_proto_library(
    name = "packets_cc_proto",
    deps = [":packets_proto"],
    visibility = ["//visibility:public"],
)


proto_library(
    name = "packets_proto",
    srcs = glob(["**/*.proto"]),
    deps = [
        "//proto/util:math_proto",
        "@com_google_protobuf//:wrappers_proto",
    ],
)