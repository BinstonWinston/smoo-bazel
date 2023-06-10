load("@rules_cc//cc:defs.bzl", "cc_binary")
load("//devkitpro-bazel:tools.bzl", "nacp", "nro")

cc_binary(
    name = "hello-world",
    srcs = ["hello-world.cpp"],
    features = ["interpret_as_executable"],
    copts = [
        "-march=armv8-a+crc+crypto",
        "-mtune=cortex-a57",
        "-mtp=soft",
        "-fPIE",
        "-g",
        "-Wall",
        "-O2",
        "-ffunction-sections",
        "-fno-rtti",
        "-fno-exceptions",
    ],
    exec_compatible_with = ["//devkitpro-bazel/devkitpro_toolchain:simple_cpp_toolchain"]
)

nacp(
    name = "hello-world-nacp",
    nacp_name = "hello-world",
    author = "IcyIceberg",
    version = "1.0.0",
    titleid = "0100000000010000",
)

nro(
    name = "hello-world-nro",
    target = ":hello-world",
)