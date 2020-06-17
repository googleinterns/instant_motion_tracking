// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <vector>
#include <memory>
#include <cmath>
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/port/ret_check.h"
#include "mediapipe/graphs/instantmotiontracking/calculators/transformations.h"

namespace mediapipe {

constexpr char kAnchorsTag[] = "ANCHORS";
constexpr char kUserScalingsTag[] = "USER_SCALINGS";
constexpr char kFinalTranslationsTag[] = "TRANSLATION_DATA";

// Adds user scalings to the translation transformation and converts normalized
// input x,y,z to OPENGL x,y,z (calculates min x and y via aspect ratio and FOV)
// Combines all X,Y,Z,scalings into the finalized translation.x, translation.y,
// and translation.z floats (usable by model matrix)
//
// Input:
//  ANCHORS - Anchors produced by tracking graph with normalized x,y,z
//  USER_SCALINGS - UserScalings from the user input
// Output:
//  TRANSLATION_DATA - Translation transformations with x,y,z in OpenGL plane
//
// Example config:
// node {
//  calculator: "TranslationManagerCalculator"
//  input_stream: "ANCHORS:tracked_scaled_anchor_data"
//  input_stream: "USER_SCALINGS:user_scaling_data"
//  output_stream: "TRANSLATION_DATA:final_translation_data"
// }

class TranslationManagerCalculator : public CalculatorBase {
 private:
  // (68 degrees, 4:3 for typical phone camera/display)
  float vertical_fov_radians_ = (68) * M_PI / 180.0;
  float aspect_ratio_ = (4.0 / 3.0);
  // initial Z value (-98 is just in visual range for OpenGL render)
  float initial_z_ = -98;

  float getUserScaling(std::vector<UserScaling> scalings, int sticker_id) {
    for (UserScaling user_scaling : scalings) {
      if (user_scaling.sticker_id == sticker_id) {
        return user_scaling.scaling_increment;
      }
    }
  }

 public:
  static ::mediapipe::Status GetContract(CalculatorContract* cc) {
    RET_CHECK(!cc -> Inputs().GetTags().empty());
    RET_CHECK(!cc -> Outputs().GetTags().empty());

    if (cc -> Inputs().HasTag(kAnchorsTag))
      cc -> Inputs().Tag(kAnchorsTag).Set<std::vector<Anchor>>();

    if (cc -> Inputs().HasTag(kUserScalingsTag))
      cc -> Inputs().Tag(kUserScalingsTag).Set<std::vector<UserScaling>>();

    if (cc -> Outputs().HasTag(kFinalTranslationsTag))
      cc -> Outputs()
          .Tag(kFinalTranslationsTag)
          .Set<std::vector<Translation>>();

    return ::mediapipe::OkStatus();
  }

  ::mediapipe::Status Open(CalculatorContext* cc) final {
    cc -> SetOffset(TimestampDiff(0));
    return ::mediapipe::OkStatus();
  }

  ::mediapipe::Status Process(CalculatorContext* cc) final {
    std::vector<Translation> combined_translation_data;
    const std::vector<UserScaling> user_scalings =
        cc -> Inputs().Tag(kUserScalingsTag).Get<std::vector<UserScaling>>();
    const std::vector<Anchor> anchors =
        cc -> Inputs().Tag(kAnchorsTag).Get<std::vector<Anchor>>();

    for (Anchor tracked_anchor : anchors) {
      Translation combined_translation;
      int id = tracked_anchor.sticker_id;
      combined_translation.sticker_id = id;
      float user_scaling = getUserScaling(user_scalings, id);

      // Convert from normalized [0.0-1.0] to openGL on-screen coordinates
      combined_translation.z =
          (initial_z_ + user_scaling) *
          tracked_anchor.z;  // translation.tracking_scaling_factor;
      float y_minimum =
          combined_translation.z *
          (tan(vertical_fov_radians_ /
               2));  // Minimum y value appearing on screen at z distance
      float x_minimum =
          y_minimum *
          (1.0 /
           aspect_ratio_);  // Minimum x value appearing on screen at z distance
      combined_translation.x =
          (tracked_anchor.x * (-x_minimum - x_minimum)) + x_minimum;
      combined_translation.y =
          (tracked_anchor.y * (-y_minimum - y_minimum)) + y_minimum;

      combined_translation_data.emplace_back(combined_translation);
    }

    if (cc -> Outputs().HasTag(kFinalTranslationsTag))
      cc -> Outputs()
          .Tag(kFinalTranslationsTag)
          .AddPacket(MakePacket<std::vector<Translation>>(
                         combined_translation_data)
                         .At(cc -> InputTimestamp()));

    return ::mediapipe::OkStatus();
  }

  ::mediapipe::Status Close(CalculatorContext* cc) final {
    return ::mediapipe::OkStatus();
  }
};

REGISTER_CALCULATOR(TranslationManagerCalculator);
}  // namespace mediapipe
