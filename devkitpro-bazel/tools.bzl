load("@rules_cc//cc:defs.bzl", "cc_binary")

load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "find_cpp_toolchain")   
load("@bazel_tools//tools/build_defs/cc:action_names.bzl", "CPP_COMPILE_ACTION_NAME") 

def _cc_binary_impl(ctx):
    # Adapted from example rule here https://github.com/bazelbuild/rules_cc/blob/main/examples/my_c_archive/my_c_archive.bzl
    output_file = ctx.actions.declare_file(ctx.label.name)
    
    srcs = ctx.files.srcs
    cc_toolchain = find_cpp_toolchain(ctx)
    tools = cc_toolchain.all_files
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

    libs = []
    for lib in ctx.attr.deps:
        if len(lib.files.to_list()) == 0:
            continue
        lib_to_link = cc_common.create_library_to_link(
            actions = ctx.actions,
            feature_configuration = feature_configuration,
            cc_toolchain = cc_toolchain,
            static_library = lib.files.to_list().files,
        )
        libs.append(lib_to_link)

    linker_input = cc_common.create_linker_input(
        owner = ctx.label,
        libraries = depset(direct = libs),
    )
    compilation_context = cc_common.create_compilation_context()
    linking_context = cc_common.create_linking_context(linker_inputs = depset(direct = [linker_input]))

    args = ctx.actions.args()
    args.add_all(compiler_options)

    ctx.actions.run(
        executable = compiler,
        arguments = [args],
        env = compiler_env,
        inputs = depset(
            direct = srcs,
            transitive = [
                cc_toolchain.all_files,
            ],
        ),
        outputs = [output_file],
    )
    cc_info = cc_common.merge_cc_infos(cc_infos = [
        CcInfo(compilation_context = compilation_context, linking_context = linking_context),
    ] + [dep[CcInfo] for dep in ctx.attr.deps])
    return [cc_info]


    # cc_binary(
    #     srcs = ctx.attr.srcs,
    #     deps = ctx.attr.deps,
    #     copts = [
    #         "-march=armv8-a+crc+crypto",
    #         "-mtune=cortex-a57",
    #         "-mtp=soft",
    #         "-fPIE",
    #         "-ftls-model=local-exec",
    #         "-g",
    #         "-Wall",
    #         "-O2",
    #         "-ffunction-sections",
    #         "-fno-rtti",
    #         "-fno-exceptions",
    #     ],
    #     linkopts = [
    #         "-specs=" + ctx.attr.switch_specs.files.to_list()[0].path,
    #         "-g",

    #         "-march=armv8-a+crc+crypto",
    #         "-mtune=cortex-a57",
    #         "-mtp=soft",
    #         "-fPIE",
    #         "-ftls-model=local-exec",
    #         # "-Wl,-Map,$(notdir $*.map)"
    #         # "-Wl,--version-script=$(TOPDIR)/exported.txt",
    #         "-Wl,-init=__custom_init -Wl,-fini=__custom_fini -nostdlib",
    #     ],
    #     data = [
    #         ctx.attr.switch_specs,
    #     ],
    #     exec_compatible_with = ["@devkitpro//devkitpro_toolchain:simple_cpp_toolchain"]
    # )

dkp_cc_binary = rule(
    implementation = _cc_binary_impl,
    attrs = {
        "srcs": attr.label_list(mandatory=True, allow_files=True),
        "deps": attr.label_list(default=[]),
        "switch_specs": attr.label(default="@devkitpro_sys//:default_switch_specs"),
        "_cc_toolchain": attr.label(default = Label("@bazel_tools//tools/cpp:current_cc_toolchain")),
    },
    toolchains = ["@bazel_tools//tools/cpp:toolchain_type"],
    fragments = ["cpp"]
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
