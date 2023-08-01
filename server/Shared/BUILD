load(
    "@rules_dotnet//dotnet:defs.bzl",
    "csharp_library",
)

csharp_library(
    name = "smoo_shared",
    srcs = glob(["**/*.cs"]),
    target_frameworks = ["net6.0"],
    visibility = ["//visibility:public"],
)