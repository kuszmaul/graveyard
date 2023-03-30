cc_binary(
    name = "hash_tables_benchmark",
    srcs = ["hash_tables_benchmark.cc"],
    deps = [
        ":enum_print",
        ":enums_flag",
        ":hash_benchmark",
        ":graveyard_set",
        ":ordered_linear_probing_set",
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
        "@com_google_absl//absl/log:check",
    ],
)

cc_library(
    name = "contains",
    hdrs = ["contains.h"],
)

cc_binary(
    name = "folly_test",
    srcs = ["folly_test.cc"],
    deps = ["@folly//folly/container:F14Set"],
)

cc_library(
    name = "graveyard_set",
    hdrs = ["graveyard_set.h"],
    deps = [
        "@com_google_absl//absl/container:flat_hash_set",
        ":hash_table",
    ],
)

cc_test(
    name = "graveyard_set_test",
    srcs = ["graveyard_set_test.cc"],
    size = "small",
    deps = [
        ":graveyard_set",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/random",
        "@com_google_googletest//:gtest_main",
    ]
)

cc_library(
    name = "graveyard_map",
    hdrs = ["graveyard_map.h"],
    deps = [
        "@com_google_absl//absl/container:flat_hash_set",
        ":hash_table",
    ],
)

cc_test(
    name = "graveyard_map_test",
    srcs = ["graveyard_map_test.cc"],
    size = "small",
    deps = [
        ":graveyard_map",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/random",
        "@com_google_googletest//:gtest_main",
    ]
)

cc_library(
    name = "object_holder",
    visibility = ["//visibility:private"],
    hdrs = ["internal/object_holder.h"],
)

cc_library(
    name = "hash_table",
    hdrs = ["internal/hash_table.h"],
    visibility = ["//visibility:private"],
    deps = [":object_holder",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
    ],
)

cc_binary(
    name = "probe_length_benchmark",
    srcs = ["internal/probe_length_benchmark.cc"],
    deps = [":graveyard_set",],
)

cc_binary(
    name = "amortization_benchmark",
    srcs = ["amortization_benchmark.cc"],
    deps = [":hash_benchmark",
            ":graveyard_set",
            "@folly//folly/container:F14Set",
            "@com_google_absl//absl/flags:flag",
    	    "@com_google_absl//absl/flags:parse",
],
)