load("@rules_proto//proto:defs.bzl", "proto_library")

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