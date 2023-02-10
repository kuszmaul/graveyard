cc_binary(
    name = "hash_tables_benchmark",
    srcs = ["hash_tables_benchmark.cc"],
    deps = [
        ":enum_print",
        ":enums_flag",
        ":hash_benchmark",
        ":tombstone_set",
        ":ordered_linear_probing_set",
        "@com_google_absl//absl/algorithm:container",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@folly//folly/container:F14Set",
    ],
)

cc_library(
    name = "ordered_linear_probing_set",
    hdrs = ["ordered_linear_probing_set.h"],
    deps = [
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
    ],
)

cc_library(
    name = "hash_benchmark",
    srcs = ["hash_benchmark.cc"],
    hdrs = ["hash_benchmark.h"],
    deps = [
        ":benchmark",
        ":enum_print",
        ":enums_flag",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "enum_print",
    hdrs = ["enum_print.h"],
    deps = [
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "enums_flag",
    hdrs = ["enums_flag.h"],
    deps = [
        ":enum_print",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "benchmark",
    srcs = ["benchmark.cc"],
    hdrs = ["benchmark.h"],
    deps = [
        "@com_google_absl//absl/flags:flag",
    ],
)

cc_library(
    name = "contains",
    hdrs = ["contains.h"],
)

cc_library(
    name = "tombstone_set",
    hdrs = ["tombstone_set.h"],
    deps = [
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
    ],
)

cc_test(
    name = "tombstone_test",
    srcs = ["tombstone_test.cc"],
    deps = [
        ":tombstone_set",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/random",
        "@com_google_googletest//:gtest_main",
    ]
)

cc_binary(
    name = "folly_test",
    srcs = ["folly_test.cc"],
    deps = ["@folly//folly/container:F14Set"],
)
