workspace(name = "com_nelhage_rubik")

load(
    "@bazel_tools//tools/build_defs/repo:git.bzl",
    "git_repository",
    "new_git_repository",
)

new_git_repository(
    name = "com_github_catchorg_catch2",
    build_file = "//third_party:catch.BUILD",
    commit = "01ef7076f50f5f2b481ddf082e1afca3c926983f",
    remote = "https://github.com/catchorg/catch2",
)

git_repository(
    name = "com_google_absl",
    commit = "3088e76c597e068479e82508b1770a7ad0c806b6",
    remote = "https://github.com/abseil/abseil-cpp",
)
