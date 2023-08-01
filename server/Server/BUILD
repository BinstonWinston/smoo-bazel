load(
    "@rules_dotnet//dotnet:defs.bzl",
    "csharp_binary",
)

csharp_binary(
    name = "smoo_server",
    srcs = glob(["**/*.cs"]),
    private_deps = [
        "//server/Shared:smoo_shared",
    ],
    target_frameworks = ["net6.0"],
    deps = [
        # ":lib",
    ],
)