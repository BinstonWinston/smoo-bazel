load("@rules_cc//cc:defs.bzl", "cc_binary")

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