# Copyright 2020 The MediaPipe Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

load("//mediapipe/framework/port:build_config.bzl", "mediapipe_cc_proto_library")

licenses(["notice"])  # Apache 2.0

package(default_visibility = ["//visibility:public"])

proto_library(
    name = "object_proto",
    srcs = [
        "object.proto",
    ],
)

proto_library(
    name = "a_r_capture_metadata_proto",
    srcs = [
        "a_r_capture_metadata.proto",
    ],
)

mediapipe_cc_proto_library(
    name = "object_cc_proto",
    srcs = ["object.proto"],
    visibility = ["//visibility:public"],
    deps = [":object_proto"],
)

mediapipe_cc_proto_library(
    name = "a_r_capture_metadata_cc_proto",
    srcs = ["a_r_capture_metadata.proto"],
    visibility = ["//visibility:public"],
    deps = [":a_r_capture_metadata_proto"],
)

mediapipe_cc_proto_library(
    name = "annotation_cc_proto",
    srcs = ["annotation_data.proto"],
    cc_deps = [
        ":a_r_capture_metadata_cc_proto",
        ":object_cc_proto",
    ],
    visibility = ["//visibility:public"],
    deps = [":annotation_proto"],
)

proto_library(
    name = "gl_animation_overlay_calculator_proto",
    srcs = ["gl_animation_overlay_calculator.proto"],
    visibility = ["//visibility:public"],
    deps = ["//mediapipe/framework:calculator_proto"],
)

proto_library(
    name = "model_matrix_proto",
    srcs = ["model_matrix.proto"],
    visibility = ["//visibility:public"],
    deps = [
        "//mediapipe/framework:calculator_proto",
    ],
)

proto_library(
    name = "camera_parameters_proto",
    srcs = [
        "camera_parameters.proto",
    ],
)

mediapipe_cc_proto_library(
    name = "camera_parameters_cc_proto",
    srcs = ["camera_parameters.proto"],
    visibility = ["//visibility:public"],
    deps = [":camera_parameters_proto"],
)

mediapipe_cc_proto_library(
    name = "gl_animation_overlay_calculator_cc_proto",
    srcs = ["gl_animation_overlay_calculator.proto"],
    cc_deps = [
        "//mediapipe/framework:calculator_cc_proto",
    ],
    visibility = ["//visibility:public"],
    deps = [":gl_animation_overlay_calculator_proto"],
)

mediapipe_cc_proto_library(
    name = "model_matrix_cc_proto",
    srcs = ["model_matrix.proto"],
    cc_deps = ["//mediapipe/framework:calculator_cc_proto"],
    visibility = ["//visibility:public"],
    deps = [":model_matrix_proto"],
)

cc_library(
    name = "box",
    srcs = [
        "box.cc",
        "model.cc",
    ],
    hdrs = [
        "box.h",
        "model.h",
        "types.h",
    ],
    deps = [
        ":annotation_cc_proto",
        ":object_cc_proto",
        "//mediapipe/framework/port:logging",
        "@eigen_archive//:eigen",
    ],
)
