SSEOPT = ["-msse -msse2 -msse3 -mssse3 -msse4 -msse4.1 -msse4.2"]

cc_library(
    name = "rubik",
    srcs = [
        "rubik.cc",
        "tables.cc",
    ],
    hdrs = [
        "rubik.h",
        "rubik_impl.h",
        "tables.h",
    ],
    copts = SSEOPT,
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
    deps = [":rubik"],
)

cc_binary(
    name = "rubik_bench",
    srcs = ["rubik_bench.cc"],
    deps = [":rubik"],
)
