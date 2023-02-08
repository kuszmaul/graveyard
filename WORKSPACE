load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

#http_archive(
#  name = "rules_cc",
#  urls = ["https://github.com/bazelbuild/rules_cc/archive/262ebec3c2296296526740db4aefce68c80de7fa.zip"],
#  strip_prefix = "rules_cc-262ebec3c2296296526740db4aefce68c80de7fa",
#)

http_archive(
  name = "com_google_absl",
  urls = ["https://github.com/abseil/abseil-cpp/archive/e1c897f09a3ae4ed76f4c17006eacaadbd8a56f9.zip"],
  strip_prefix = "abseil-cpp-e1c897f09a3ae4ed76f4c17006eacaadbd8a56f9",
)

http_archive(
  name = "bazel_skylib",
  urls = ["https://github.com/bazelbuild/bazel-skylib/releases/download/1.2.1/bazel-skylib-1.2.1.tar.gz"],
  sha256 = "f7be3474d42aae265405a592bb7da8e171919d74c16f082a5457840f06054728",
)

http_archive(
  name = "com_google_googletest",
  urls = ["https://github.com/google/googletest/archive/b73f27fd164456fea9aba56163f5511355a03272.zip"],
  strip_prefix = "googletest-b73f27fd164456fea9aba56163f5511355a03272",
)

http_archive(
    name = "com_github_google_benchmark",
    urls = ["https://github.com/google/benchmark/archive/bf585a2789e30585b4e3ce6baf11ef2750b54677.zip"],
    strip_prefix = "benchmark-bf585a2789e30585b4e3ce6baf11ef2750b54677",
    sha256 = "2a778d821997df7d8646c9c59b8edb9a573a6e04c534c01892a40aa524a7b68c",
)

local_repository(
    name = "folly",
    path = "/home/bradleybear/github/folly",
)
