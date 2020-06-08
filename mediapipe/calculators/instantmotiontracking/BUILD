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
    name = "sticker",
    srcs = ["sticker.h"],
    visibility = [
        "//visibility:public",
    ],
)

cc_library(
    name = "sticker_data_manager_calculator",
    srcs = ["sticker_data_manager_calculator.cc"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":sticker",
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
    name = "model_matrix_manager_calculator",
    srcs = ["model_matrix_manager_calculator.cc"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
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
  ":sticker",
    ],
    alwayslink = 1,
)

cc_library(
    name = "gl_animation_overlay_calculator",
    srcs = ["gl_animation_overlay_calculator.cc"],
    visibility = ["//visibility:public"],
    deps = [
        "//mediapipe/graphs/object_detection_3d/calculators:camera_parameters_cc_proto",
        "//mediapipe/graphs/object_detection_3d/calculators:gl_animation_overlay_calculator_cc_proto",
        "//mediapipe/graphs/object_detection_3d/calculators:model_matrix_cc_proto",
        "//mediapipe/framework:calculator_framework",
        "//mediapipe/framework/port:ret_check",
        "//mediapipe/framework/port:status",
        "//mediapipe/gpu:gl_calculator_helper",
        "//mediapipe/gpu:shader_util",
        "//mediapipe/util/android:asset_manager_util",
    ],
    alwayslink = 1,
)
