load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "find_cpp_toolchain")   
load("@bazel_tools//tools/build_defs/cc:action_names.bzl", "CPP_COMPILE_ACTION_NAME", "CPP_LINK_DYNAMIC_LIBRARY_ACTION_NAME") 

def _dkp_header_only_cc_library_impl(ctx):
    return [DefaultInfo(files = depset(ctx.files.srcs + ctx.files.deps))]


dkp_header_only_cc_library = rule(
    implementation = _dkp_header_only_cc_library_impl,
    attrs = {
        "srcs": attr.label_list(mandatory=True, allow_files=True),
        "deps": attr.label_list(default=[]),
    }
)

SourceFiles = provider(
    fields = {
        "transitive_source_files": "list of transitive source files of a target",
    },
)

#add 'resources' ? if so _accumulate_transitive_config_files needs to check for dep in deps if ConfigFiles in dep
SourceAttr = ["tars", "deps", "runtime_deps", "exports"]

def _accumulate_transitive_source_files(accumulated, deps):
    return depset(
        transitive = [dep[SourceFiles].transitive_source_files for dep in deps] + [accumulated],
    )

def _collect_current_source_files(srcs):
    return [file for src in srcs for file in src.files.to_list()]

def _collect_source_files_aspect_impl(target, ctx):
    current_source_files = []
    if hasattr(ctx.rule.attr, "srcs"):
        current_source_files = _collect_current_source_files(ctx.rule.attr.srcs)
    if hasattr(ctx.rule.attr, "resources"):
        current_source_files = current_source_files + _collect_current_source_files(ctx.rule.attr.resources)

    accumulated_source_files = depset(current_source_files)
    for attr in SourceAttr:
        if hasattr(ctx.rule.attr, attr):
            accumulated_source_files = _accumulate_transitive_source_files(accumulated_source_files, getattr(ctx.rule.attr, attr))

    return [SourceFiles(transitive_source_files = accumulated_source_files)]

collect_source_files_aspect = aspect(
    implementation = _collect_source_files_aspect_impl,
    attr_aspects = SourceAttr,
)

def _dkp_cc_library_impl(ctx):
    cc_toolchain = find_cpp_toolchain(ctx)
    feature_configuration = cc_common.configure_features(
        ctx = ctx,
        cc_toolchain = cc_toolchain,
        unsupported_features = ctx.disabled_features,
    )
    compiler = cc_common.get_tool_for_action(
        feature_configuration=feature_configuration,
        action_name=CPP_COMPILE_ACTION_NAME
    )
    compile_variables = cc_common.create_compile_variables(
        feature_configuration = feature_configuration,
        cc_toolchain = cc_toolchain,
    )
    compiler_options = cc_common.get_memory_inefficient_command_line(
        feature_configuration = feature_configuration,
        action_name = CPP_COMPILE_ACTION_NAME,
        variables = compile_variables,
    )
    compiler_env = cc_common.get_environment_variables(
        feature_configuration = feature_configuration,
        action_name = CPP_COMPILE_ACTION_NAME,
        variables = compile_variables,
    )
    linker = cc_common.get_tool_for_action(
        feature_configuration=feature_configuration,
        action_name=CPP_LINK_DYNAMIC_LIBRARY_ACTION_NAME
    )
    link_variables = cc_common.create_link_variables(
        feature_configuration = feature_configuration,
        cc_toolchain = cc_toolchain,
    )
    link_options = cc_common.get_memory_inefficient_command_line(
        feature_configuration = feature_configuration,
        action_name = CPP_LINK_DYNAMIC_LIBRARY_ACTION_NAME,
        variables = compile_variables,
    )
    linker_env = cc_common.get_environment_variables(
        feature_configuration = feature_configuration,
        action_name = CPP_LINK_DYNAMIC_LIBRARY_ACTION_NAME,
        variables = link_variables,
    )

    compile_flags = "-c -gdwarf-2 -gstrict-dwarf -D__SWITCH__ -DSWITCH -DNNSDK -DSMOVER=100 -std=gnu++20 -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE -ftls-model=local-exec -g -Wall -O3 -ffunction-sections -fno-rtti -fno-exceptions -Wno-invalid-offsetof -Wno-volatile -fno-rtti -fomit-frame-pointer -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables"
    for include_path in ctx.attr.includes:
        compile_flags += ' -I{include_path}'.format(include_path = include_path)
    compile_flags += ' -DEMU={emu}'.format(emu = 1 if ctx.attr.emu else 0)

    link_flags = "-specs=/specs/switch.specs -g -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE -ftls-model=local-exec -Wl,-Map,/patches/maps/100/main.map -Wl,--version-script=/patches/exported.txt -Wl,-init=__custom_init -Wl,-fini=__custom_fini -nostdlib"
    link_flags += " -Xlinker -Map={output_map}".format(output_map = ctx.outputs.map.path)

    print(ctx.attr.deps[0].files)

    include_deps = []
    for dep in ctx.attr.deps:
        compilation_context = dep[CcInfo].compilation_context
        include_deps += compilation_context.system_includes.to_list()
        for include_path in compilation_context.system_includes.to_list():
            compile_flags += ' -isystem {include_path}'.format(include_path = include_path)
        include_deps += compilation_context.quote_includes.to_list()
        for include_path in compilation_context.quote_includes.to_list():
            compile_flags += ' -iquote {include_path}'.format(include_path = include_path)
        include_deps += compilation_context.includes.to_list()
        for include_path in compilation_context.includes.to_list():
            compile_flags += ' -I{include_path}'.format(include_path = include_path)
    include_files = []
    for include_dep in include_deps:
        include_files += [Label(include_dep)]
    include_deps = include_files

    print(compile_flags)

    dep_lib = ctx.attr.deps[0][CcInfo].linking_context
    print(dep_lib)
    print(dir(dep_lib))

    dep_files_without_shared_libs = []
    dep_lib_files = []
    dep_lib_paths = []
    include_paths = []
    for dep_file in ctx.files.deps:
        if dep_file.extension != 'so':
            dep_files_without_shared_libs.append(dep_file)
        if dep_file.path.endswith('.pb.h'):
            compile_flags += ' -I{include_path}'.format(include_path = dep_file.dirname)
        elif dep_file.extension in ['elf', 'a']:
            dep_lib_files.append(dep_file)
            dep_lib_paths.append(dep_file.path)

    for include_path in ctx.attr.includes + include_paths:
        compile_flags += ' -I{include_path}'.format(include_path = include_path)

    my_deps = []
    for dep in ctx.attr.deps:
        my_deps += dep[SourceFiles].transitive_source_files.to_list()

    obj_files = []
    obj_paths = []
    for src_file in ctx.files.srcs:
        obj_file = ctx.actions.declare_file("%s.o" % src_file.basename)
        obj_files.append(obj_file)
        obj_paths.append(obj_file.path)

        # Compile.
        compile_args = ctx.actions.args()
        compile_args.add_all(compiler_options)
        ctx.actions.run_shell(
            outputs = [obj_file],
            inputs = [src_file] + ctx.files.private_hdrs + ctx.files.public_hdrs + dep_files_without_shared_libs + my_deps,
            command = "{compiler} {cc_flags} {src_file} -o {obj_file}".format(
                compiler = compiler,
                cc_flags = compile_flags,
                src_file = src_file.path,
                obj_file = obj_file.path,
            ),
            arguments = [compile_args],
            env = compiler_env,
        )

    # Link.
    link_args = ctx.actions.args()
    link_args.add_all(link_options)
    ctx.actions.run_shell(
        outputs = [ctx.outputs.elf, ctx.outputs.map],
        inputs = obj_files + dep_lib_files,
        command = "{linker} {link_flags} {obj_files} {dep_lib_files} -o {out_elf_file}".format(
            linker = linker,
            obj_files = obj_paths,
            dep_lib_files = dep_lib_paths,
            link_flags = link_flags,
            out_elf_file = ctx.outputs.elf.path,
        ),
        arguments = [link_args],
        env = linker_env,
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
        "deps": attr.label_list(default = [], aspects = [collect_source_files_aspect], providers = [CcInfo]),
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
    map_file = ctx.files.target[1] # Map file is second output from dkp_cc_binary rule
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
        "target": attr.label(mandatory=True),
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
