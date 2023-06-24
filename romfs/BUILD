
romfs_files = glob(["**/*"])
romfs_files.remove("BUILD")
filegroup(
    name = "smoo_romfs",
    srcs = romfs_files,
    visibility = ["//visibility:public"]
)