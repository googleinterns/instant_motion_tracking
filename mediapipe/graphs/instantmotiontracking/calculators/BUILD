# Copyright 2020 Google LLC
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

exports_files(["LICENSE"])

cc_library(
    name = "transformations",
    srcs = ["transformations.h"],
    alwayslink = 1,
)

cc_library(
    name = "sticker_manager_calculator",
    srcs = ["sticker_manager_calculator.cc"],
    deps = [
        ":transformations",
        "//mediapipe/framework:calculator_framework",
        "//mediapipe/framework/port:integral_types",
        "//mediapipe/framework/port:logging",
        "//mediapipe/framework/port:ret_check",
        "//mediapipe/framework/port:source_location",
        "//mediapipe/framework/port:status",
        "//mediapipe/framework:timestamp",
    ],
    alwayslink = 1,
)

cc_library(
    name = "translation_manager_calculator",
    srcs = ["translation_manager_calculator.cc"],
    deps = [
        ":transformations",
        "//mediapipe/framework:calculator_framework",
        "//mediapipe/framework/port:integral_types",
        "//mediapipe/framework/port:logging",
        "//mediapipe/framework/port:ret_check",
        "//mediapipe/framework/port:source_location",
        "//mediapipe/framework/port:status",
        "//mediapipe/framework:timestamp",
    ],
    alwayslink = 1,
)

cc_library(
    name = "matrices_manager",
    srcs = ["matrices_manager.cc"],
    deps = [
        ":transformations",
        "//mediapipe/framework:calculator_framework",
      	"//mediapipe/framework/port:integral_types",
      	"//mediapipe/framework/port:logging",
      	"//mediapipe/framework/port:opencv_imgproc",
      	"//mediapipe/framework/port:ret_check",
      	"//mediapipe/framework/port:source_location",
      	"//mediapipe/framework/port:status",
      	"//mediapipe/framework/formats:detection_cc_proto",
      	"//mediapipe/graphs/object_detection_3d/calculators:model_matrix_cc_proto",
      	"//mediapipe/graphs/object_detection_3d/calculators:box",
      	"//mediapipe/framework:timestamp",
      	"//mediapipe/framework/formats:image_frame",
      	"//mediapipe/framework/formats:image_frame_opencv",
      	"@eigen_archive//:eigen",
    ],
    alwayslink = 1,
)
