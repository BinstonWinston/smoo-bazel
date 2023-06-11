load("@devkitpro//:tools.bzl", "dkp_cc_binary", "dkp_nacp", "dkp_nro")

dkp_cc_binary(
    name = "hello-world",
    srcs = ["hello-world.cpp"]
)

dkp_nacp(
    name = "hello-world-nacp",
    nacp_name = "hello-world",
    author = "IcyIceberg",
    version = "1.0.0",
    titleid = "0100000000010000",
)

dkp_nro(
    name = "hello-world-nro",
    target = ":hello-world",
)