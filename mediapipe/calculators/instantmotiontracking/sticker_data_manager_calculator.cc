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

#include "absl/strings/str_cat.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/port/ret_check.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/gpu/gpu_buffer.h"
#include "sticker.h"

namespace mediapipe {

// This calculator takes in the sticker data from the application as a string
// in a sequence of the following format:
// "(sticker_id:1,sticker_anchor_x:1.44,sticker_anchor_y:0.0,
// sticker_rotation:0.0,sticker_scaling:0.0,sticker_render_id:0,
// should_reset_anchor:true)(sticker_id:2.............."
//
// Input:
//  STRING - String of sticker data in appropriate parse format
// Output:
//  STICKERS - Vector of Sticker objects translated from data string
//
// Example config:
// node {
//   calculator: "StickerDataManagerCalculator"
//   input_stream: "STRING:sticker_data_string"
//   output_stream: "STICKERS:sticker_data"
// }

class StickerDataManagerCalculator : public CalculatorBase {
 public:
  static ::mediapipe::Status GetContract(CalculatorContract* cc) {
    RET_CHECK(!cc->Inputs().GetTags().empty());
    RET_CHECK(!cc->Outputs().GetTags().empty());

    if (cc->Inputs().HasTag("STRING"))
      cc->Inputs().Tag("STRING").Set<std::string>();

    if (cc->Outputs().HasTag("STICKERS"))
      cc->Outputs().Tag("STICKERS").Set<std::vector<Sticker>>();

    return ::mediapipe::OkStatus();
  }

  ::mediapipe::Status Open(CalculatorContext* cc) final {
    cc->SetOffset(TimestampDiff(0));

    return ::mediapipe::OkStatus();
  }

  ::mediapipe::Status Process(CalculatorContext* cc) final {
    const std::string& sticker_data_string = cc->Inputs().Tag("STRING").Get<std::string>();
    std::vector<Sticker> stickers = setupAllStickers(sticker_data_string);

    if (cc->Outputs().HasTag("STICKERS"))
      cc->Outputs().Tag("STICKERS")
        .AddPacket(MakePacket<std::vector<Sticker>>(stickers)
        .At(cc->InputTimestamp()));

    return ::mediapipe::OkStatus();
  }

  ::mediapipe::Status Close(CalculatorContext* cc) final {
    return ::mediapipe::OkStatus();
  }

  // Convert the sticker string data directly to a vector of Sticker objects
  std::vector<Sticker> setupAllStickers(std::string stickerDataString) {
    std::vector<Sticker> stickers;
    while(stickerDataString.find(")") != -1) {
      Sticker sticker;
      std::string stickerString = stickerDataString.substr(stickerDataString.find("(")+1,stickerDataString.find(")"));
      stickerString = findPastKey("sticker_id:", stickerString);
      sticker.stickerID = std::stof(stickerString.substr(0,stickerString.find(",")));
      stickerString = findPastKey("sticker_anchor_x:", stickerString);
      sticker.xAnchor = std::stof(stickerString.substr(0,stickerString.find(",")));
      stickerString = findPastKey("sticker_anchor_y:", stickerString);
      sticker.yAnchor = std::stof(stickerString.substr(0,stickerString.find(",")));
      stickerString = findPastKey("sticker_rotation:", stickerString);
      sticker.userRotation = std::stof(stickerString.substr(0,stickerString.find(",")));
      stickerString = findPastKey("sticker_scaling:", stickerString);
      sticker.userScaling = std::stof(stickerString.substr(0,stickerString.find(",")));
      stickerString = findPastKey("sticker_render_id:", stickerString);
      sticker.renderID = std::stoi(stickerString.substr(0,stickerString.length()));
      stickerDataString = stickerDataString.substr(stickerDataString.find(")")+1, stickerDataString.length());
      stickers.push_back(sticker);
    }
    return stickers;
  }

  // Returns a string of every character after 'key'
  std::string findPastKey(std::string key, std::string original) {
    return original.substr(original.find(key)+key.length(),original.length());
  }
};

REGISTER_CALCULATOR(StickerDataManagerCalculator);
}  // namespace mediapipe
