load("@rules_cc//cc:defs.bzl", "cc_binary")

def _switch_specs_to_linker_flags_impl(ctx):
    # print(ctx.expand_location("$(location " + str(ctx.attr.switch_spec.label) + ")", targets=[ctx.attr.switch_spec]))
    spec_file_path = ctx.attr.switch_spec.files.to_list()[0].path

    linker_input = cc_common.create_linker_input(
        owner = ctx.label,
        user_link_flags = depset(["-Wl,specs=" + spec_file_path]),
    )
    compilation_context = cc_common.create_compilation_context()
    linking_context = cc_common.create_linking_context(linker_inputs = depset(direct = [linker_input]))
    return CcInfo(compilation_context = compilation_context, linking_context = linking_context)

switch_specs_to_linker_flags = rule(
    implementation = _switch_specs_to_linker_flags_impl,
    attrs = {
        "switch_spec": attr.label(mandatory=True),
        "linker_scripts": attr.label_list(default=[]),
    }
)

def dkp_cc_binary(name, srcs, deps=[], switch_specs="@devkitpro//specs:default_switch_specs", visibility=None):
    cc_binary(
        name = name,
        srcs = srcs,
        deps = deps + [switch_specs],
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
