load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_cc//examples:experimental_cc_shared_library.bzl", "cc_shared_library")

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

def _copy_file(ctx, in_file, out_file):
    ctx.actions.run_shell(
        inputs = [in_file],
        outputs = [out_file],
        progress_message = "Copying {in_file} to {out_file}".format(in_file=in_file.path, out_file=out_file.path),
        command = "mkdir -p `dirname {out_file}` && cp {in_file} {out_file}".format
                (
                in_file = in_file.path,
                out_file = out_file.path,
                ),
    )

def _atmosphere_package_impl(ctx):
    nso_file = ctx.files.nso[0]
    ips_file = ctx.files.ips_patch[0]
    in_romfs = ctx.files.romfs

    metadata_file = ctx.actions.declare_file('atmosphere/metadata.txt')
    out_romfs_relative_path = 'atmosphere/contents/{title_id}/romfs'.format(title_id=ctx.attr.title_id)
    out_exefs_relative_path = 'atmosphere/contents/{title_id}/exefs'.format(title_id=ctx.attr.title_id)
    subsdk1_file = ctx.actions.declare_file("{exefs_dir}/subsdk1".format(exefs_dir=out_exefs_relative_path))
    out_ips_file = ctx.actions.declare_file("atmosphere/exefs_patches/StarlightBase/{build_id}.ips".format(build_id=ctx.attr.build_id))

    ctx.actions.run_shell(
        inputs = [],
        outputs = [metadata_file],
        progress_message = "Creating Atmosphere directory",
        command = "echo '' > {metadata_file}".format
                (
                metadata_file = metadata_file.path
                ),
    )

    _copy_file(ctx, nso_file, subsdk1_file)
    _copy_file(ctx, ips_file, out_ips_file)

    out_romfs_files = []
    for romfs_file in in_romfs:
        filepath_relative_to_romfs_root = romfs_file.path.split('romfs/')[-1]
        out_romfs_file = ctx.actions.declare_file(out_romfs_relative_path + '/' + filepath_relative_to_romfs_root)
        _copy_file(ctx, romfs_file, out_romfs_file)
        out_romfs_files.append(out_romfs_file)

    ctx.actions.run_shell(
        inputs = [metadata_file, subsdk1_file, out_ips_file] + out_romfs_files,
        outputs = [ctx.outputs.tarball],
        progress_message = "Compressing Atmosphere package %s" % ctx.label.name,
        command = "tar -czf '%s' --dereference -C `dirname '%s'` ." %
                  (
                    ctx.outputs.tarball.path,
                    metadata_file.dirname, # Atmosphere dir
                  ),
    )

    return [DefaultInfo(files = depset([ctx.outputs.tarball]))]


dkp_atmosphere_package = rule(
    implementation = _atmosphere_package_impl,
    attrs = {
        "nso": attr.label(mandatory=True),
        "ips_patch": attr.label(mandatory=True),
        "title_id": attr.string(mandatory=True),
        "romfs": attr.label(mandatory=True, allow_files=True),
        "build_id": attr.string(mandatory=True),
    },
    outputs = {
        "tarball": "%{name}.tar.gz",
    },
)
