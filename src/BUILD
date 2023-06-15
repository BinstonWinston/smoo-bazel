load("@devkitpro//:tools.bzl", "dkp_cc_binary", "dkp_nacp", "dkp_nro")

genrule(
    name = "my_test_target",
    srcs = ["//specs:smo_switch_spec_file"],
    outs = ["app_output"],
    cmd = "echo `pwd`/$(rootpath //specs:smo_switch_spec_file) > $@",
)

dkp_cc_binary(
    name = "hello_world",
    srcs = ["hello_world.cpp"],
    deps = ["@devkitpro_sys//:libnx"],
    switch_specs = "//specs:smo_switch_specs",
)

dkp_nacp(
    name = "hello_world_nacp",
    nacp_name = "hello_world",
    author = "IcyIceberg",
    version = "1.0.0",
    titleid = "0100000000010000",
)

dkp_nro(
    name = "hello_world_nro",
    target = ":hello_world",
    nacp = ":hello_world_nacp",
)