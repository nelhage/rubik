workspace(name = "com_nelhage_rubik")

load(
    "@bazel_tools//tools/build_defs/repo:git.bzl",
    "git_repository",
    "new_git_repository",
)

new_git_repository(
    name = "com_github_catchorg_catch2",
    build_file = "//third_party:catch.BUILD",
    commit = "03d122a35c3f5c398c43095a87bc82ed44642516",  # v2.4.2
    remote = "https://github.com/catchorg/catch2",
)

git_repository(
    name = "com_google_absl",
    commit = "3088e76c597e068479e82508b1770a7ad0c806b6",
    remote = "https://github.com/abseil/abseil-cpp",
)
