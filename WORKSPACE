workspace(name="com_nelhage_rubik")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "new_git_repository")

new_git_repository(
    name = "com_github_catchorg_catch2",
    remote = "https://github.com/catchorg/catch2",
    commit = "03d122a35c3f5c398c43095a87bc82ed44642516", # v2.4.2
    build_file = "//third_party:catch.BUILD",
)
