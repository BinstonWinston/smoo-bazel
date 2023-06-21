def _dkp_header_only_cc_library_impl(ctx):
    return [DefaultInfo(files = depset(ctx.files.srcs + ctx.files.deps))]


dkp_header_only_cc_library = rule(
    implementation = _dkp_header_only_cc_library_impl,
    attrs = {
        "srcs": attr.label_list(mandatory=True, allow_files=True),
        "deps": attr.label_list(default=[]),
    }
)

def _link_input_header_file(ctx, input_header_file):
    header_file = ctx.actions.declare_file(input_header_file.path)
    ctx.actions.symlink(output=header_file, target_file=input_header_file)

def _dkp_cc_library_impl(ctx):
    # Adapted from https://github.com/jdtaylor7/bazel_blinky/blob/master/src/rules.bzl
    toolchain_path = "@bazel_tools//tools/cpp:toolchain_type"
    toolchain_provider = ctx.toolchains[toolchain_path].cc

    compiler_path = "/opt/devkitpro/devkitA64/bin/aarch64-none-elf-g++"
    linker_path = "/opt/devkitpro/devkitA64/bin/aarch64-none-elf-g++"

    compile_flags = "-c -gdwarf-2 -gstrict-dwarf -D__SWITCH__ -DSWITCH -DNNSDK -DSMOVER=100 -std=gnu++20 -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE -ftls-model=local-exec -g -Wall -O3 -ffunction-sections -fno-rtti -fno-exceptions -Wno-invalid-offsetof -Wno-volatile -fno-rtti -fomit-frame-pointer -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables"
    for include_path in ctx.attr.includes:
        compile_flags += ' -I{include_path}'.format(include_path = include_path)
    compile_flags += ' -DEMU={emu}'.format(emu = 1 if ctx.attr.emu else 0)

    link_flags = "-specs=/specs/switch.specs -g -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE -ftls-model=local-exec -Wl,-Map,/patches/maps/100/main.map -Wl,--version-script=/patches/exported.txt -Wl,-init=__custom_init -Wl,-fini=__custom_fini -nostdlib"

    dep_files = ctx.files.deps
    dep_paths = []
    for dep_file in dep_files:
        if dep_file.extension != 'elf':
            continue
        dep_paths.append(dep_file.path)

    obj_files = []
    obj_paths = []
    for src_file in ctx.files.srcs:
        obj_file = ctx.actions.declare_file("%s.o" % src_file.basename)
        obj_files.append(obj_file)
        obj_paths.append(obj_file.path)

        # Compile.
        ctx.actions.run_shell(
            outputs = [obj_file],
            inputs = [src_file] + ctx.files.private_hdrs + ctx.files.public_hdrs + dep_files,
            command = "{compiler} {copts} {lib_paths} {cc_bin} -o {obj_file}".format(
                compiler = compiler_path,
                copts = compile_flags,
                lib_paths = ' '.join(dep_paths),
                cc_bin = src_file.path,
                obj_file = obj_file.path,
            ),
        )

    # Link.
    ctx.actions.run_shell(
        outputs = [ctx.outputs.elf],
        inputs = obj_files,
        command = "{compiler} {linkopts} {obj_in} -o {elf_out}".format(
            compiler = compiler_path,
            linkopts = link_flags,
            obj_in = ' '.join(obj_paths),
            elf_out = ctx.outputs.elf.path,
        )
    )

    # Pass through public header files for targets that depend on this
    return [DefaultInfo(files = depset([ctx.outputs.elf] + ctx.files.public_hdrs))]

dkp_cc_library = rule(
    implementation = _dkp_cc_library_impl,
    fragments = ["cpp"],
    attrs = {
        "srcs": attr.label_list(mandatory = True, allow_files = True),
        "private_hdrs": attr.label_list(default = [], allow_files = True),
        "public_hdrs": attr.label_list(default = [], allow_files = True),
        "deps": attr.label_list(default = []),
        "includes": attr.string_list(default = []),
        "emu": attr.bool(mandatory = True),
    },
    outputs = {
        "elf": "%{name}.elf",
    },
    toolchains = ["@bazel_tools//tools/cpp:toolchain_type"]
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
