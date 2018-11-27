SSEOPT = ["-msse -msse2 -msse3 -mssse3 -msse4 -msse4.1 -msse4.2"]

cc_library(
    name = "rubik_core",
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

cc_binary(
    name = "gen_tables",
    srcs = [
        "search.cc",
        "tables.h",
        "tools/gen_tables.cc",
    ],
    copts = SSEOPT,
    includes = ["."],
    deps = [
        ":rubik_core",
    ],
)

cc_library(
    name = "tables",
    srcs = ["tables.cc"],
    hdrs = ["tables.h"],
    includes = ["."],
)

genrule(
    name = "run_gen_tables",
    outs = ["tables.cc"],
    cmd = "$(location :gen_tables) > $@",
    tools = [":gen_tables"],
)

cc_library(
    name = "rubik",
    srcs = ["search.cc"],
    copts = SSEOPT,
    deps = [
        ":rubik_core",
        ":tables",
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
    srcs = [
        "quad01.cc",
        "rubik_scratch.cc",
    ],
    copts = SSEOPT,
    deps = [
        ":rubik",
    ],
)

cc_binary(
    name = "rubik_bench",
    srcs = ["rubik_bench.cc"],
    deps = [
        ":rubik",
        "@com_google_absl//absl/types:optional",
    ],
)
