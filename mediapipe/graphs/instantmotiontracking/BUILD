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

licenses(["notice"])  # Apache 2.0

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "mobile_calculators",
    deps = [
        "//mediapipe/graphs/instantmotiontracking/calculators:sticker_manager_calculator",
        "//mediapipe/graphs/instantmotiontracking/subgraphs:region_tracking",
        "//mediapipe/graphs/instantmotiontracking/calculators:translation_manager_calculator",
        "//mediapipe/graphs/instantmotiontracking/calculators:rotation_manager_calculator",
        "//mediapipe/graphs/instantmotiontracking/calculators:matrices_manager",
        "//mediapipe/graphs/object_detection_3d/calculators:gl_animation_overlay_calculator",
    ],
)

load(
    "//mediapipe/framework/tool:mediapipe_graph.bzl",
    "mediapipe_binary_graph",
)

mediapipe_binary_graph(
    name = "mobile_binary_graph",
    graph = "instantmotiontracking.pbtxt",
    output_name = "mobile.binarypb",
)
