SSEOPT=["-msse -msse2 -msse3 -mssse3 -msse4 -msse4.1 -msse4.2"]

cc_library(
    name = "rubik",
    hdrs = ["rubik.h", "rubik_impl.h", "tables.h"],
    srcs = ["rubik.cc", "tables.cc"],
    copts = SSEOPT,
)

cc_library(
    name = "test_main",
    hdrs = ["catch/catch.hpp"],
    srcs = ["test_main.cc"],
)

cc_test(
    name = "rubik_test",
    deps = [":test_main", ":rubik"],
    srcs = ["rubik_test.cc"],
)

cc_binary(
    name = "rubik_scratch",
    deps = [":rubik"],
    srcs = ["rubik_scratch.cc"],
    copts = SSEOPT,
)

cc_binary(
    name = "rubik_bench",
    deps = [":rubik"],
    srcs = ["rubik_bench.cc"],
)
