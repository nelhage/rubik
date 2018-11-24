SSEOPT = ["-msse -msse2 -msse3 -mssse3 -msse4 -msse4.1 -msse4.2"]

cc_library(
    name = "rubik",
    srcs = [
        "rubik.cc",
    ],
    hdrs = [
        "rubik.h",
        "rubik_impl.h",
    ],
    copts = SSEOPT,
    includes = ["."],
    linkopts = ["-pthread"],
    deps = [
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:variant",
    ],
)

cc_library(
    name = "test_main",
    srcs = ["test_main.cc"],
    deps = ["@com_github_catchorg_catch2//:catch"],
)

cc_test(
    name = "rubik_test",
    srcs = ["rubik_test.cc"],
    deps = [
        ":rubik",
        ":test_main",
        "@com_github_catchorg_catch2//:catch",
    ],
)

cc_binary(
    name = "rubik_scratch",
    srcs = ["rubik_scratch.cc"],
    copts = SSEOPT,
    deps = [
        ":rubik",
        ":tables",
    ],
)

cc_binary(
    name = "rubik_bench",
    srcs = ["rubik_bench.cc"],
    deps = [":rubik"],
)

cc_binary(
    name = "gen_tables",
    srcs = ["tools/gen_tables.cc"],
    copts = SSEOPT,
    deps = [":rubik"],
)

cc_library(
    name = "tables",
    srcs = [":build_tables"],
    hdrs = ["tables.h"],
    includes = ["."],
)

genrule(
    name = "build_tables",
    outs = ["tables.cc"],
    cmd = "$(location :gen_tables) > $@",
    tools = [":gen_tables"],
)
