load("@devkitpro//:tools.bzl", "switch_specs_to_linker_flags")

switch_specs_to_linker_flags(
    name = "smo_switch_specs",
    switch_spec = ":smo_switch_spec_file",
    linker_scripts = [":smo_linker_scripts"],
    visibility = ["//visibility:public"],
)

filegroup(
    name = "smo_switch_spec_file",
    srcs = ["switch.specs"],
    visibility = ["//visibility:public"],
)

filegroup(
    name = "smo_linker_scripts",
    srcs = glob([
        "linkerscripts/*.ld",
    ]),
)