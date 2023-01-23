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
          ":benchmark",
  ],
)

cc_library(
    name = "simple_integer_linear_probing",
    hdrs = ["simple_integer_linear_probing.h"],
)

cc_library(
    name = "benchmark",
    hdrs = ["benchmark.h",],
)
