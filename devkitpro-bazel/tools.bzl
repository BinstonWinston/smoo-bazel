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
    link_flags += " -Xlinker -Map={output_map}".format(output_map = ctx.outputs.map.path)

    dep_files = ctx.files.deps
    dep_paths = []
    for dep_file in dep_files:
        print(dep_file)
        if dep_file.extension not in ['elf', 'cc']: # Include cc for protobuf files
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
        outputs = [ctx.outputs.elf, ctx.outputs.map],
        inputs = obj_files,
        command = "{compiler} {linkopts} {obj_in} -o {elf_out}".format(
            compiler = compiler_path,
            linkopts = link_flags,
            obj_in = ' '.join(obj_paths),
            elf_out = ctx.outputs.elf.path,
        )
    )

    # Pass through public header files for targets that depend on this
    return [DefaultInfo(files = depset([ctx.outputs.elf, ctx.outputs.map] + ctx.files.public_hdrs))]

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
        "map": "%{name}.map",
    },
    toolchains = ["@bazel_tools//tools/cpp:toolchain_type"]
)

def _nso_impl(ctx):
    in_elf_file = ctx.attr.target.files.to_list()[0]
    out_file = ctx.actions.declare_file("%s.nso" % ctx.attr.name)
    ctx.actions.run_shell(
        inputs = [in_elf_file],
        outputs = [out_file],
        progress_message = "Creating nso file %s" % ctx.label.name,
        command = "/opt/devkitpro/tools/bin/elf2nso '%s' '%s'" %
                  (in_elf_file.path,
                  out_file.path),
    )

    return [DefaultInfo(files = depset([out_file]))]


dkp_nso = rule(
    implementation = _nso_impl,
    attrs = {
        "target": attr.label(mandatory=True),
    }
)

def _ips_patch_impl(ctx):
    gen_patch_script = ctx.files._gen_patch_script[0]
    map_file = ctx.files.binary[1] # Map file is second file output from dkp_cc_binary rule
    ctx.actions.run_shell(
        inputs = [gen_patch_script, map_file],
        outputs = [ctx.outputs.ips],
        progress_message = "Creating IPS patch file %s" % ctx.label.name,
        command = "python3 '%s' '%s' '%s' '%s'" %
                  (
                  gen_patch_script.path,
                  ctx.attr.version,
                  map_file.path,
                  ctx.outputs.ips.path,
                  ),
    )

    return [DefaultInfo(files = depset([ctx.outputs.ips]))]


dkp_ips_patch = rule(
    implementation = _ips_patch_impl,
    attrs = {
        "version": attr.string(mandatory=True),
        "build_id": attr.string(mandatory=True),
        "binary": attr.label(mandatory=True),
        "_gen_patch_script": attr.label(default="//scripts:gen_patch"),
    },
    outputs = {
        "ips": "%{name}/%{build_id}.ips",
    },
)

def _atmosphere_package_impl(ctx):
    nso_file = ctx.files.nso[0]
    ips_file = ctx.files.ips_patch[0]
    in_romfs_dir = ctx.files.romfs[0]
    atmosphere_dir = ctx.actions.declare_directory("atmosphere")
    exefs_dir = ctx.actions.declare_directory("atmosphere/contents/{title_id}/exefs".format(title_id=ctx.attr.title_id))
    exefs_patches_dir = ctx.actions.declare_directory("atmosphere/exefs_patches/StarlightBase")
    out_romfs_dir = ctx.actions.declare_directory("atmosphere/contents/{title_id}/romfs".format(title_id=ctx.attr.title_id))
    subsdk1_file = ctx.actions.declare_file("atmosphere/contents/{title_id}/exefs/subsdk1".format(title_id=ctx.attr.title_id))
    ctx.actions.run_shell(
        inputs = [nso_file, ips_file, in_romfs_dir],
        outputs = [subsdk1_file, exefs_dir, exefs_patches_dir, out_romfs_dir, atmosphere_dir],
        progress_message = "Creating Atmosphere package %s" % ctx.label.name,
        command = "cp {nso_file} {subsdk_file} && cp {in_ips} {exefs_patches_dir} && cp -r {in_romfs} {out_romfs}".format
                  (
                  nso_file = nso_file.path,
                  subsdk_file = subsdk1_file.path,
                  in_ips = ips_file.path,
                  exefs_patches_dir = exefs_patches_dir.path,
                  in_romfs = in_romfs_dir.path,
                  out_romfs = out_romfs_dir.path,
                  ),
    )
    ctx.actions.run_shell(
        inputs = [subsdk1_file, exefs_dir, out_romfs_dir, atmosphere_dir],
        outputs = [ctx.outputs.tarball],
        progress_message = "Compressing Atmosphere package %s" % ctx.label.name,
        command = "tar -czf '%s' --dereference -C '%s' ." %
                  (
                    ctx.outputs.tarball.path,
                    atmosphere_dir.dirname, # Parent dir that atmosphere dir is in
                  ),
    )

    return [DefaultInfo(files = depset([ctx.outputs.tarball]))]


dkp_atmosphere_package = rule(
    implementation = _atmosphere_package_impl,
    attrs = {
        "nso": attr.label(mandatory=True),
        "ips_patch": attr.label(mandatory=True),
        "title_id": attr.string(mandatory=True),
        "romfs": attr.label(mandatory=True, allow_single_file=True),
    },
    outputs = {
        "tarball": "%{name}.tar.gz",
    },
)
