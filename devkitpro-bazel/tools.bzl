load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

def dkp_cc_binary(name, srcs, deps=[], visibility=None):
    cc_binary(
        name = name,
        srcs = srcs,
        deps = deps,
        copts = [
            "-std=c++20",
            "-march=armv8-a+crc+crypto",
            "-mtune=cortex-a57",
            "-mtp=soft",
            "-fPIE",
            "-ftls-model=local-exec",
            "-g",
            "-Wall",
            "-O2",
            "-ffunction-sections",
            "-fno-rtti",
            "-fno-exceptions",
        ],
        linkopts = [
            "-specs=/specs/switch.specs",
            "-g",

            "-march=armv8-a+crc+crypto",
            "-mtune=cortex-a57",
            "-mtp=soft",
            "-fPIE",
            "-ftls-model=local-exec",
            # "-Wl,-Map,$(notdir $*.map)"
            # "-Wl,--version-script=$(TOPDIR)/exported.txt",
            "-Wl,-init=__custom_init -Wl,-fini=__custom_fini -nostdlib",
        ],
        exec_compatible_with = ["@devkitpro//devkitpro_toolchain:simple_cpp_toolchain"]
    )

def dkp_cc_library(name, srcs, hdrs, deps=[], visibility=None):
    cc_library(
        name = name,
        srcs = srcs,
        hdrs = hdrs,
        deps = deps,
        copts = [
            "-std=c++20",
            "-march=armv8-a+crc+crypto",
            "-mtune=cortex-a57",
            "-mtp=soft",
            "-fPIE",
            "-ftls-model=local-exec",
            "-g",
            "-Wall",
            "-O2",
            "-ffunction-sections",
            "-fno-rtti",
            "-fno-exceptions",
        ],
        linkopts = [
            "-specs=/specs/switch.specs",
            "-g",

            "-march=armv8-a+crc+crypto",
            "-mtune=cortex-a57",
            "-mtp=soft",
            "-fPIE",
            "-ftls-model=local-exec",
            # "-Wl,-Map,$(notdir $*.map)"
            # "-Wl,--version-script=$(TOPDIR)/exported.txt",
            "-Wl,-init=__custom_init -Wl,-fini=__custom_fini -nostdlib",
        ],
        exec_compatible_with = ["@devkitpro//devkitpro_toolchain:simple_cpp_toolchain"]
    )

def _nacp_impl(ctx):
    out_file = ctx.actions.declare_file("%s.nacp" % ctx.attr.name)
    ctx.actions.run_shell(
        inputs = [],
        outputs = [out_file],
        progress_message = "Creating NACP file %s" % ctx.label.name,
        command = "/opt/devkitpro/tools/bin/nacptool --create '%s' '%s' '%s' '%s' --titleid='%s'" %
                  (ctx.attr.nacp_name,
                  ctx.attr.author,
                  ctx.attr.version,
                  out_file.path,
                  ctx.attr.titleid),
    )

    return [DefaultInfo(files = depset([out_file]))]

dkp_nacp = rule(
    implementation = _nacp_impl,
    attrs = {
        "nacp_name": attr.string(mandatory=True),
        "author": attr.string(mandatory=True),
        "version": attr.string(mandatory=True),
        "titleid": attr.string(mandatory=True),
    }
)


def _nro_impl(ctx):
    in_elf_file = ctx.attr.target.files.to_list()[0]
    in_nacp_file = ctx.attr.nacp.files.to_list()[0]
    out_file = ctx.actions.declare_file("%s.nro" % ctx.attr.name)
    ctx.actions.run_shell(
        inputs = [in_elf_file, in_nacp_file,],
        outputs = [out_file],
        progress_message = "Creating NRO file %s" % ctx.label.name,
        command = "/opt/devkitpro/tools/bin/elf2nro '%s' '%s' --nacp '%s'" %
                  (in_elf_file.path,
                  out_file.path,
                  in_nacp_file.path),
    )

    return [DefaultInfo(files = depset([out_file]))]


dkp_nro = rule(
    implementation = _nro_impl,
    attrs = {
        "target": attr.label(mandatory=True),
        "nacp": attr.label(mandatory=True),
    }
)
