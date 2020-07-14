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
#include "mediapipe/graphs/instantmotiontracking/calculators/sticker_buffer.pb.h"

namespace mediapipe {

constexpr char kProtoDataString[] = "PROTO";
constexpr char kAnchorsTag[] = "ANCHORS";
constexpr char kUserTransformsTag[] = "USER_TRANSFORMS";
constexpr char kRenderDescriptorsTag[] = "RENDER_DATA";

// This calculator takes in the sticker protobuffer data and parses each individual
// sticker object into anchors, user rotations and scalings, in addition to basic
// render data represented in integer form.
//
// Input:
//  PROTO - String of sticker data in appropriate protobuf format [REQUIRED]
// Output:
//  ANCHORS - Anchors with initial normalized X,Y coordinates [REQUIRED]
//  USER_TRANSFORMS - UserTransforms with sticker rotation and scale factor from user [REQUIRED]
//  RENDER_DATA - Descriptors of which objects/animations to render for stickers [REQUIRED]
//
// Example config:
// node {
//   calculator: "StickerManagerCalculator"
//   input_stream: "PROTO:sticker_proto_string"
//   output_stream: "ANCHORS:initial_anchor_data"
//   output_stream: "USER_TRANSFORMS:user_transform_data"
//   output_stream: "RENDER_DATA:sticker_render_data"
// }

class StickerManagerCalculator : public CalculatorBase {
 public:
  static ::mediapipe::Status GetContract(CalculatorContract* cc) {
    RET_CHECK(cc->Inputs().HasTag(kProtoDataString));
    RET_CHECK(cc->Outputs().HasTag(kAnchorsTag)
      && cc->Outputs().HasTag(kUserTransformsTag)
      && cc->Outputs().HasTag(kRenderDescriptorsTag));

    if (cc->Inputs().HasTag(kProtoDataString)) {
      cc->Inputs().Tag(kProtoDataString).Set<std::string>();
    }
    if (cc->Outputs().HasTag(kAnchorsTag)) {
      cc->Outputs().Tag(kAnchorsTag).Set<std::vector<Anchor>>();
    }
    if (cc->Outputs().HasTag(kUserTransformsTag)) {
      cc->Outputs().Tag(kUserTransformsTag).Set<std::vector<UserTransform>>();
    }
    if (cc->Outputs().HasTag(kRenderDescriptorsTag)) {
      cc->Outputs()
          .Tag(kRenderDescriptorsTag)
          .Set<std::vector<int>>();
    }

    return ::mediapipe::OkStatus();
  }

  ::mediapipe::Status Open(CalculatorContext* cc) final {
    cc->SetOffset(TimestampDiff(0));
    return ::mediapipe::OkStatus();
  }

  ::mediapipe::Status Process(CalculatorContext* cc) final {
    std::string sticker_proto_string =
      cc->Inputs().Tag(kProtoDataString).Get<std::string>();

    std::vector<Anchor> initial_anchor_data;
    std::vector<UserTransform> user_transform_data;
    std::vector<int> render_data;

    instantmotiontracking::StickerRoll sticker_roll;
    bool parse_success = sticker_roll.ParseFromString(sticker_proto_string);

    // Ensure parsing was a success
    RET_CHECK(parse_success) << "Error parsing sticker protobuf data";

    for (int i = 0; i < sticker_roll.sticker().size(); i++) {
      // Declare empty structures for sticker data
      Anchor initial_anchor;
      UserTransform user_transform;
      // Get individual Sticker object as defined by Protobuffer
      instantmotiontracking::Sticker sticker = sticker_roll.sticker(i);
      // Set individual data structure ids to associate with this sticker
      initial_anchor.sticker_id = sticker.id();
      user_transform.sticker_id = sticker.id();
      initial_anchor.x = sticker.x();
      initial_anchor.y = sticker.y();
      initial_anchor.z = 1.0; // default to 1.0 in normalized 3d space
      user_transform.rotation_radians = sticker.rotation();
      user_transform.scale_factor = sticker.scale();
      float render_id = sticker.renderid();
      // Set all vector data with sticker attributes
      initial_anchor_data.emplace_back(initial_anchor);
      user_transform_data.emplace_back(user_transform);
      render_data.emplace_back(render_id);
    }

    if (cc->Outputs().HasTag(kAnchorsTag)) {
      cc->Outputs()
          .Tag(kAnchorsTag)
          .AddPacket(MakePacket<std::vector<Anchor>>(initial_anchor_data)
                         .At(cc->InputTimestamp()));
    }
    if (cc->Outputs().HasTag(kUserTransformsTag)) {
      cc->Outputs()
          .Tag(kUserTransformsTag)
          .AddPacket(MakePacket<std::vector<UserTransform>>(user_transform_data)
                         .At(cc->InputTimestamp()));
    }
    if (cc->Outputs().HasTag(kRenderDescriptorsTag)) {
      cc->Outputs()
          .Tag(kRenderDescriptorsTag)
          .AddPacket(
              MakePacket<std::vector<int>>(render_data)
                  .At(cc->InputTimestamp()));
    }

    return ::mediapipe::OkStatus();
  }

  ::mediapipe::Status Close(CalculatorContext* cc) final {
    return ::mediapipe::OkStatus();
  }
};

REGISTER_CALCULATOR(StickerManagerCalculator);
}  // namespace mediapipe
