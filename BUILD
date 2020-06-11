# Copyright 2018, OpenCensus Authors
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

load("@com_github_grpc_grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")
load("@rules_proto//proto:defs.bzl", "proto_library")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_proto_library", "cc_library")
load("@io_bazel_rules_docker//cc:image.bzl", "cc_image")
load("@io_bazel_rules_docker//container:container.bzl", "container_push")

licenses(["notice"])  # Apache License 2.0

package(default_visibility = ["//visibility:public"])

proto_library(
    name = "supplyfinder_proto",
    srcs = ["proto/supplyfinder.proto"],
    deps = ["@com_google_protobuf//:empty_proto"],
)

cc_proto_library(
    name = "supplyfinder_cc_proto",
    deps = [":supplyfinder_proto"],
)

cc_grpc_library(
    name = "supplyfinder_cc_grpc",
    srcs = [":supplyfinder_proto"],
    grpc_only = True,
    deps = [":supplyfinder_cc_proto"],
)

cc_library(
    name = "helpers",
    srcs = ["helpers.cc"],
    hdrs = ["helpers.h"],
    defines = ["BAZEL_BUILD"],
    deps = [
        ":supplyfinder_cc_grpc",
    ],
)

cc_library(
    name = "exporters",
    srcs = ["exporters.cc"],
    hdrs = ["exporters.h"],
    deps = [
        "@io_opencensus_cpp//opencensus/exporters/stats/stackdriver:stackdriver_exporter",
        "@io_opencensus_cpp//opencensus/exporters/stats/stdout:stdout_exporter",
        "@io_opencensus_cpp//opencensus/exporters/trace/ocagent:ocagent_exporter",
        "@io_opencensus_cpp//opencensus/exporters/trace/stackdriver:stackdriver_exporter",
        "@io_opencensus_cpp//opencensus/exporters/trace/stdout:stdout_exporter",
        "@com_google_absl//absl/strings",
    ],
)

cc_binary(
    name = "supplyfinder_finder",
    srcs = ["finder/finder.cc", "finder/finder.h"],
    defines = ["BAZEL_BUILD"],
    deps = [
        ":exporters",
        ":supplyfinder_cc_grpc",
        ":supplyfinder_cc_proto",
        "@io_opencensus_cpp//opencensus/tags",
        "@io_opencensus_cpp//opencensus/tags:context_util",
        "@io_opencensus_cpp//opencensus/trace",
        "@io_opencensus_cpp//opencensus/trace:context_util",
        "@io_opencensus_cpp//opencensus/trace:with_span",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc_opencensus_plugin",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)

cc_binary(
    name = "supplyfinder_supplier",
    srcs = ["supplier/supplier.cc"],
    defines = ["BAZEL_BUILD"],
    deps = [
        ":helpers",
        ":exporters",
        ":supplyfinder_cc_grpc",
        ":supplyfinder_cc_proto",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc_opencensus_plugin",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)

cc_binary(
    name = "supplyfinder_vendor",
    srcs = ["vendor/vendor.cc"],
    defines = ["BAZEL_BUILD"],
    deps = [
        ":helpers",
        ":exporters",
        ":supplyfinder_cc_grpc",
        ":supplyfinder_cc_proto",
        "@io_opencensus_cpp//opencensus/tags",
        "@io_opencensus_cpp//opencensus/tags:context_util",
        "@io_opencensus_cpp//opencensus/trace",
        "@io_opencensus_cpp//opencensus/trace:context_util",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc_opencensus_plugin",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)

cc_binary(
    name = "supplyfinder_client",
    srcs = ["client/client.cc"],
    defines = ["BAZEL_BUILD"],
    deps = [
        ":helpers",
        ":exporters",
        ":supplyfinder_cc_grpc",
        ":supplyfinder_cc_proto",
        "@io_opencensus_cpp//opencensus/tags",
        "@io_opencensus_cpp//opencensus/tags:context_util",
        "@io_opencensus_cpp//opencensus/trace",
        "@io_opencensus_cpp//opencensus/trace:context_util",
        "@io_opencensus_cpp//opencensus/trace:with_span",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc_opencensus_plugin",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)

cc_image(
    name = "client_image",
    binary = ":supplyfinder_client",
)