load("@rules_proto//proto:defs.bzl", "proto_library")

cc_library(
    name = "test1",
    srcs = ["test1.cpp"],
    hdrs = ["test1.h"],
)

cc_library(
    name = "test2",
    srcs = ["test2.cpp"],
    hdrs = ["test2.h"],
    deps = [":test1"],
    visibility = ["//visibility:public"],
)

cc_proto_library(
    name = "test_packet_cc_proto",
    deps = [":test_packet_proto"],
    exec_compatible_with = ["@devkitpro//devkitpro_toolchain:simple_cpp_toolchain"],
    visibility = ["//visibility:public"],
)


proto_library(
    name = "test_packet_proto",
    srcs = ["test_packet.proto"],
    deps = [
        # "@com_google_protobuf//:any_proto",
    ],
)