# /opt/devkitpro/tools/bin/nacptool --create <name> <author> <version> <outfile> [options]

# FLAGS:
# --create : Create control.nacp for use with Switch homebrew applications.
# Options:
# --titleid=<titleID>

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

nacp = rule(
    implementation = _nacp_impl,
    attrs = {
        "nacp_name": attr.string(),
        "author": attr.string(),
        "version": attr.string(),
        "titleid": attr.string(),
    }
)


def _nro_impl(ctx):
    out = ctx.actions.declare_file(ctx.label.name)
    ctx.actions.write(
        output = out,
        content = "Hello {}!\n".format(ctx.attr.target),
    )
    return [DefaultInfo(files = depset([out]))]


nro = rule(
    implementation = _nro_impl,
    attrs = {
        "target": attr.label(),
    }
)
