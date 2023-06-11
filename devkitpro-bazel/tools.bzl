load("@rules_cc//cc:defs.bzl", "cc_binary")

def dkp_cc_binary(name, srcs, switch_specs="@devkitpro_sys//:default_switch_specs", visibility=None):
    cc_binary(
        name = name,
        srcs = srcs,
        copts = [
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
            "-specs=/opt/devkitpro/libnx/switch.specs",
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
        data = [
            switch_specs,
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
    in_file = ctx.attr.target.files.to_list()[0]
    out_file = ctx.actions.declare_file("%s.nro" % ctx.attr.name)
    ctx.actions.run_shell(
        inputs = [in_file],
        outputs = [out_file],
        progress_message = "Creating NRO file %s" % ctx.label.name,
        command = "/opt/devkitpro/tools/bin/elf2nro '%s' '%s'" %
                  (in_file.path,
                  out_file.path),
    )

    return [DefaultInfo(files = depset([out_file]))]


dkp_nro = rule(
    implementation = _nro_impl,
    attrs = {
        "target": attr.label(mandatory=True),
    }
)
