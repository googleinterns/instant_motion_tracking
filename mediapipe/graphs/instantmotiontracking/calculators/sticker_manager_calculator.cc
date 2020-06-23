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
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/port/ret_check.h"
#include "mediapipe/graphs/instantmotiontracking/calculators/transformations.h"

namespace mediapipe {

constexpr char kStringTag[] = "STRING";
constexpr char kAnchorsTag[] = "ANCHORS";
constexpr char kUserRotationsTag[] = "USER_ROTATIONS";
constexpr char kUserScalingsTag[] = "USER_SCALINGS";
constexpr char kRenderDescriptorsTag[] = "RENDER_DATA";

// This calculator takes in the sticker data from the application as a string
// in a sequence of the following format:
// "(sticker_id:1,sticker_anchor_x:1.44,sticker_anchor_y:0.0,
// sticker_rotation:0.0,sticker_scaling:0.0,sticker_render_id:0,
// should_reset_anchor:true)(sticker_id:2.............."
//
// Input:
//  STRING - String of sticker data in appropriate parse format
// Output:
//  ANCHORS - Anchors with initial normalized X,Y coordinates
//  USER_ROTATIONS - UserRotations with radians of rotation from user
//  USER_SCALINGS - UserScalings with increment of scaling from user
//  RENDER_DATA - Descriptors of which objects/animations to render for stickers
//
// Example config:
// node {
//   calculator: "StickerManagerCalculator"
//   input_stream: "STRING:sticker_data_string"
//   output_stream: "ANCHORS:initial_anchor_data"
//   output_stream: "USER_ROTATIONS:user_rotation_data"
//   output_stream: "USER_SCALINGS:user_scaling_data"
//   output_stream: "RENDER_DATA:sticker_render_data"
// }

class StickerManagerCalculator : public CalculatorBase {
 public:
  static ::mediapipe::Status GetContract(CalculatorContract* cc) {
    RET_CHECK(!cc->Inputs().GetTags().empty());
    RET_CHECK(!cc->Outputs().GetTags().empty());

    if (cc->Inputs().HasTag(kStringTag)) {
      cc->Inputs().Tag(kStringTag).Set<std::string>();
    }
    if (cc->Outputs().HasTag(kAnchorsTag)) {
      cc->Outputs().Tag(kAnchorsTag).Set<std::vector<Anchor>>();
    }
    if (cc->Outputs().HasTag(kUserRotationsTag)) {
      cc->Outputs().Tag(kUserRotationsTag).Set<std::vector<UserRotation>>();
    }
    if (cc->Outputs().HasTag(kUserScalingsTag)) {
      cc->Outputs().Tag(kUserScalingsTag).Set<std::vector<UserScaling>>();
    }
    if (cc->Outputs().HasTag(kRenderDescriptorsTag)) {
      cc->Outputs()
          .Tag(kRenderDescriptorsTag)
          .Set<std::vector<RenderDescriptor>>();
    }

    return ::mediapipe::OkStatus();
  }

  ::mediapipe::Status Open(CalculatorContext* cc) final {
    cc->SetOffset(TimestampDiff(0));
    return ::mediapipe::OkStatus();
  }

  ::mediapipe::Status Process(CalculatorContext* cc) final {
    std::string sticker_data_string =
        cc->Inputs().Tag(kStringTag).Get<std::string>();

    std::vector<Anchor> initial_anchor_data;
    std::vector<UserRotation> user_rotation_data;
    std::vector<UserScaling> user_scaling_data;
    std::vector<RenderDescriptor> render_descriptor_data;

    while (sticker_data_string.find(")") != -1) {
      Anchor initial_anchor;
      UserRotation user_rotation;
      UserScaling user_scaling;
      RenderDescriptor render_descriptor;
      std::string stickerString = sticker_data_string.substr(
          sticker_data_string.find("(") + 1, sticker_data_string.find(")"));

      // Associate all data with a single sticker ID value
      stickerString = findPastKey("sticker_id:", stickerString);
      int sticker_id =
          std::stoi(stickerString.substr(0, stickerString.find(",")));
      initial_anchor.sticker_id = sticker_id;
      user_rotation.sticker_id = sticker_id;
      user_scaling.sticker_id = sticker_id;
      render_descriptor.sticker_id = sticker_id;

      // Set the initial anchor data
      stickerString = findPastKey("sticker_anchor_x:", stickerString);
      initial_anchor.x =
          std::stof(stickerString.substr(0, stickerString.find(",")));
      stickerString = findPastKey("sticker_anchor_y:", stickerString);
      initial_anchor.y =
          std::stof(stickerString.substr(0, stickerString.find(",")));
      initial_anchor.z = 1.0;  // default normalized z-value

      // Set user rotation data
      stickerString = findPastKey("sticker_rotation:", stickerString);
      user_rotation.radians =
          std::stof(stickerString.substr(0, stickerString.find(",")));

      // Set user scaling data
      stickerString = findPastKey("sticker_scaling:", stickerString);
      user_scaling.scaling_increment =
          std::stof(stickerString.substr(0, stickerString.find(",")));

      // Set render data
      stickerString = findPastKey("sticker_render_id:", stickerString);
      render_descriptor.render_object_id =
          std::stoi(stickerString.substr(0, stickerString.length()));
      sticker_data_string = sticker_data_string.substr(
          sticker_data_string.find(")") + 1, sticker_data_string.length());

      // Add all data parsed sticker data to appropriate vector
      initial_anchor_data.emplace_back(initial_anchor);
      user_rotation_data.emplace_back(user_rotation);
      user_scaling_data.emplace_back(user_scaling);
      render_descriptor_data.emplace_back(render_descriptor);
    }

    if (cc->Outputs().HasTag(kAnchorsTag)) {
      cc->Outputs()
          .Tag(kAnchorsTag)
          .AddPacket(MakePacket<std::vector<Anchor>>(initial_anchor_data)
                         .At(cc->InputTimestamp()));
    }
    if (cc->Outputs().HasTag(kUserRotationsTag)) {
      cc->Outputs()
          .Tag(kUserRotationsTag)
          .AddPacket(MakePacket<std::vector<UserRotation>>(user_rotation_data)
                         .At(cc->InputTimestamp()));
    }
    if (cc->Outputs().HasTag(kUserScalingsTag)) {
      cc->Outputs()
          .Tag(kUserScalingsTag)
          .AddPacket(MakePacket<std::vector<UserScaling>>(user_scaling_data)
                         .At(cc->InputTimestamp()));
    }
    if (cc->Outputs().HasTag(kRenderDescriptorsTag)) {
      cc->Outputs()
          .Tag(kRenderDescriptorsTag)
          .AddPacket(
              MakePacket<std::vector<RenderDescriptor>>(render_descriptor_data)
                  .At(cc->InputTimestamp()));
    }

    return ::mediapipe::OkStatus();
  }

  ::mediapipe::Status Close(CalculatorContext* cc) final {
    return ::mediapipe::OkStatus();
  }

  // Returns a string of every character after 'key'
  std::string findPastKey(std::string key, std::string original) {
    return original.substr(original.find(key) + key.length(),
                           original.length());
  }
};

REGISTER_CALCULATOR(StickerManagerCalculator);
}  // namespace mediapipe
