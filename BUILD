cc_test(
  name = "simple_integer_graveyard_test",
  size = "small",
  srcs = ["simple_integer_graveyard_test.cc"],
  deps = ["@com_google_googletest//:gtest_main"],
)

cc_binary(
  name = "simple_integer_linear_probing_benchmark",
  srcs = ["simple_integer_linear_probing_benchmark.cc"],
  deps = [":simple_integer_linear_probing",
          ":hash_benchmark",
  ],
)

cc_library(
    name = "simple_integer_linear_probing",
    hdrs = ["simple_integer_linear_probing.h"],
)

cc_library(
    name = "hash_benchmark",
    hdrs = ["hash_benchmark.h",],
    srcs = ["hash_benchmark.cc",],
    deps = [
        ":benchmark",
        ":contains",
    ],
)

cc_library(
    name = "benchmark",
    hdrs = ["benchmark.h",],
)

cc_library(
    name = "contains",
    hdrs = ["contains.h",],
)
