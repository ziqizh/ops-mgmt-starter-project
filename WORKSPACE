# Copyright 2017, OpenCensus Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")
http_archive(
    name = "io_opencensus_cpp",
    strip_prefix = "opencensus-cpp-master",
    urls = ["https://github.com/census-instrumentation/opencensus-cpp/archive/master.zip"],
)

maybe(
    http_archive,
    name = "rules_cc",
    strip_prefix = "rules_cc-master",
    urls = ["https://github.com/bazelbuild/rules_cc/archive/master.zip"],
)

# We depend on Abseil.
maybe(
    http_archive,
    name = "com_google_absl",
    strip_prefix = "abseil-cpp-master",
    urls = ["https://github.com/abseil/abseil-cpp/archive/master.zip"],
)

# gRPC
maybe(
    http_archive,
    name = "com_github_grpc_grpc",
    strip_prefix = "grpc-master",
    urls = ["https://github.com/grpc/grpc/archive/master.tar.gz"],
)

# Prometheus client library - used by Prometheus exporter.
maybe(
    http_archive,
    name = "com_github_jupp0r_prometheus_cpp",
    strip_prefix = "prometheus-cpp-master",
    urls = ["https://github.com/jupp0r/prometheus-cpp/archive/master.zip"],
)

# Google APIs - used by Stackdriver exporter.
maybe(
    http_archive,
    name = "com_google_googleapis",
    strip_prefix = "googleapis-master",
    urls = ["https://github.com/googleapis/googleapis/archive/master.zip"],
)

# Needed by opencensus-proto.
#maybe(
#    http_archive,
#    name = "io_bazel_rules_go",
#    sha256 = "9fb16af4d4836c8222142e54c9efa0bb5fc562ffc893ce2abeac3e25daead144",
#    urls = [
#        "https://storage.googleapis.com/bazel-mirror/github.com/bazelbuild/rules_go/releases/download/0.19.0/rules_go-0.19.0.tar.gz",
#        "https://github.com/bazelbuild/rules_go/releases/download/0.19.0/rules_go-0.19.0.tar.gz",
#    ],
#)

# OpenCensus proto - used by OcAgent exporter.
maybe(
    http_archive,
    name = "opencensus_proto",
    strip_prefix = "opencensus-proto-master/src",
    urls = ["https://github.com/census-instrumentation/opencensus-proto/archive/master.zip"],
)


# GoogleTest framework.
# Only needed for tests, not to build the OpenCensus library.
http_archive(
    name = "com_google_googletest",
    strip_prefix = "googletest-master",
    urls = ["https://github.com/google/googletest/archive/master.zip"],
)

# Google Benchmark library.
# Only needed for benchmarks, not to build the OpenCensus library.
http_archive(
    name = "com_github_google_benchmark",
    strip_prefix = "benchmark-master",
    urls = ["https://github.com/google/benchmark/archive/master.zip"],
)

load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")

grpc_deps()

# grpc_deps() cannot load() its deps, this WORKSPACE has to do it.
# See also: https://github.com/bazelbuild/bazel/issues/1943
load(
    "@build_bazel_rules_apple//apple:repositories.bzl",
    "apple_rules_dependencies",
)

apple_rules_dependencies()

load(
    "@build_bazel_apple_support//lib:repositories.bzl",
    "apple_support_dependencies",
)

apple_support_dependencies()

load("@upb//bazel:repository_defs.bzl", "bazel_version_repository")

bazel_version_repository(name = "upb_bazel_version")

# Used by prometheus-cpp.
local_repository(
    name = "net_zlib_zlib",
    path = "tools/zlib",
)

# Load Prometheus dependencies.
load("@com_github_jupp0r_prometheus_cpp//bazel:repositories.bzl", "prometheus_cpp_repositories")

prometheus_cpp_repositories()

# Google APIs - used by Stackdriver exporter.
load("@com_google_googleapis//:repository_rules.bzl", "switched_rules_by_language")

switched_rules_by_language(
    name = "com_google_googleapis_imports",
    cc = True,
    grpc = True,
)

# Needed by @opencensus_proto.
# load("@io_bazel_rules_go//go:deps.bzl", "go_register_toolchains", "go_rules_dependencies")
# 
# go_rules_dependencies()
# 
# go_register_toolchains()

http_archive(
    name = "rules_proto",
    sha256 = "602e7161d9195e50246177e7c55b2f39950a9cf7366f74ed5f22fd45750cd208",
    strip_prefix = "rules_proto-97d8af4dc474595af3900dd85cb3a29ad28cc313",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/rules_proto/archive/97d8af4dc474595af3900dd85cb3a29ad28cc313.tar.gz",
        "https://github.com/bazelbuild/rules_proto/archive/97d8af4dc474595af3900dd85cb3a29ad28cc313.tar.gz",
    ],
)
load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")
rules_proto_dependencies()
rules_proto_toolchains()

load("@rules_cc//cc:repositories.bzl", "rules_cc_dependencies")
rules_cc_dependencies()

# Setup Docker

http_archive(
    name = "io_bazel_rules_docker",
    sha256 = "6287241e033d247e9da5ff705dd6ef526bac39ae82f3d17de1b69f8cb313f9cd",
    strip_prefix = "rules_docker-0.14.3",
    urls = ["https://github.com/bazelbuild/rules_docker/releases/download/v0.14.3/rules_docker-v0.14.3.tar.gz"],
)

load(
    "@io_bazel_rules_docker//repositories:repositories.bzl",
    container_repositories = "repositories",
)

container_repositories()

load(
    "@io_bazel_rules_docker//cc:image.bzl",
    _cc_image_repos = "repositories",
)

_cc_image_repos()