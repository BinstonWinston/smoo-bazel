load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_cc//examples:experimental_cc_shared_library.bzl", "cc_shared_library")

def _dkp_header_only_cc_library_impl(ctx):
    return [DefaultInfo(files = depset(ctx.files.srcs + ctx.files.deps))]


dkp_header_only_cc_library = rule(
    implementation = _dkp_header_only_cc_library_impl,
    attrs = {
        "srcs": attr.label_list(mandatory=True, allow_files=True),
        "deps": attr.label_list(default=[]),
    }
)

def dkp_cc_library(name, srcs, emu, hdrs = [], includes = [], copts = [], deps = [], visibility = None):
    cc_library(
        name = "{name}_lib".format(name=name),
        srcs = srcs,
        hdrs = hdrs,
        includes = includes,
        copts = copts,
        defines = [
            "EMU={emu}".format(emu = 1 if (emu == True) else 0) # == True is intentional to type-check the boolean
        ],
        deps = deps,
    )
    cc_library(
        name = "{name}_linker_map".format(name=name),
        srcs = srcs,
        hdrs = hdrs,
        includes = includes,
        copts = copts,
        linkopts = ['--special-link-opt-generate-linker-map'],
        defines = [
            "EMU={emu}".format(emu = 1 if (emu == True) else 0) # == True is intentional to type-check the boolean
        ],
        deps = deps,
    )
    cc_shared_library(
        name = name,
        deps = [":{name}_lib".format(name=name)],
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
    print(ctx.files.linker_map)
    map_file = ctx.files.linker_map[1] # Second file in cc_library rule output is .so and that is replaced to the linker map file by g++-wrapper.py
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
        "linker_map": attr.label(mandatory=True),
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
