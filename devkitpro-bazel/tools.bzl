load("@rules_cc//cc:defs.bzl", "cc_binary")

def _dkp_cc_binary_impl(ctx):
    out_binary = ctx.actions.declare_file("%s" % ctx.attr.name)
    cc_args = [
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
        "-fno-exceptions"
    ]
    linker_args = [
        "-Wl,-spec=`pwd`/" + ctx.files.switch_specs[0].path,
        # "-Wl,-Map,$(notdir $*.map)"
        # "-Wl,--version-script=$(TOPDIR)/exported.txt",
        "-Wl,-init=__custom_init -Wl,-fini=__custom_fini -nostdlib",
    ]
    # for i in range(len(linker_args)):
    #     linker_args[i] = "-Wl," + linker_args[i]
    src_files = []
    for file in ctx.files.srcs:
        src_files.append(file.path)
    lib_files = []
    for file in ctx.attr.deps:
        print(file[CcInfo].to_json())
        lib_files.append(file.path)
    print(lib_files)
    ctx.actions.run_shell(
        inputs = ctx.files.srcs + ctx.files.switch_specs + ctx.files.switch_linker_scripts,
        outputs = [out_binary],
        progress_message = "Creating dkp_cc_binary %s" % ctx.label.name,
        # $(CC)
        command = "/opt/devkitpro/devkitA64/bin/aarch64-none-elf-g++ %s %s %s -o '%s'" %
                  (
                  " ".join(cc_args),
                  " ".join(linker_args),
                  " ".join(lib_files),
                  " ".join(src_files),
                  out_binary.path),
    )

    return [DefaultInfo(files = depset([out_binary]))]

dkp_cc_binary = rule(
    implementation = _dkp_cc_binary_impl,
    attrs = {
        "srcs": attr.label_list(mandatory=True, allow_files=True),
        "deps": attr.label_list(default=[]),
        "switch_specs": attr.label(default="@devkitpro_sys//:default_switch_spec_file"),
        "switch_linker_scripts": attr.label_list(default=[]),
    }
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
