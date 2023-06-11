load("@devkitpro//:tools.bzl", "switch_specs_to_linker_flags")

switch_specs_to_linker_flags(
    name = "smo_switch_specs",
    switch_spec_files = ":smo_switch_spec_files",
    visibility = ["//visibility:public"],
)

filegroup(
    name = "smo_switch_spec_files",
    srcs = ["switch.specs"] + glob([
        "linkerscripts/*.ld",
    ]),
)