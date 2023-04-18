load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

#http_archive(
#  name = "rules_cc",
#  urls = ["https://github.com/bazelbuild/rules_cc/archive/262ebec3c2296296526740db4aefce68c80de7fa.zip"],
#  strip_prefix = "rules_cc-262ebec3c2296296526740db4aefce68c80de7fa",
#)

#http_archive(
#  name = "com_google_absl",
#  # 2023-03-21
#  urls = ["https://github.com/abseil/abseil-cpp/archive/819272485a0c06abc8d7d62b188a6f54174881cb.zip"],
#  strip_prefix = "abseil-cpp-819272485a0c06abc8d7d62b188a6f54174881cb",
#  sha256 = "9c76997179607994e1fd5715f51d578dcf8c110f641b5638a52288cd4e8129ca",
#)
local_repository(
 name = "com_google_absl",
 path = "/home/bradley/github/abseil-cpp",
)

http_archive(
  name = "bazel_skylib",
  urls = ["https://github.com/bazelbuild/bazel-skylib/releases/download/1.2.1/bazel-skylib-1.2.1.tar.gz"],
  sha256 = "f7be3474d42aae265405a592bb7da8e171919d74c16f082a5457840f06054728",
)

#http_archive(
#  name = "com_google_googletest",
#  urls = ["https://github.com/google/googletest/archive/b73f27fd164456fea9aba56163f5511355a03272.zip"],
#  strip_prefix = "googletest-b73f27fd164456fea9aba56163f5511355a03272",

#http_archive(
#    name = "com_github_google_benchmark",
#    urls = ["https://github.com/google/benchmark/archive/bf585a2789e30585b4e3ce6baf11ef2750b54677.zip"],
#    strip_prefix = "benchmark-bf585a2789e30585b4e3ce6baf11ef2750b54677",
#    sha256 = "2a778d821997df7d8646c9c59b8edb9a573a6e04c534c01892a40aa524a7b68c",
#)
local_repository(
   name = "com_google_googletest",
   path = "/home/bradley/github/googletest",
)

local_repository(
    name = "folly",
    path = "/home/bradley/github/folly",
)

local_repository(
    name = "libcuckoo",
    path = "/home/bradley/github/libcuckoo",
    )
    