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

#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/detection.pb.h"
#include "mediapipe/framework/formats/location_data.pb.h"
#include "mediapipe/framework/port/status.h"
#include "mediapipe/framework/port/ret_check.h"
#include "mediapipe/util/tracking/box_tracker.h"
#include "mediapipe/graphs/instantmotiontracking/calculators/transformations.h"

namespace mediapipe {

constexpr char kAnchorsTag[] = "ANCHORS";
constexpr char kBoxesInputTag[] = "BOXES";
constexpr char kBoxesOutputTag[] = "START_POS";
constexpr char kCancelTag[] = "CANCEL_ID";
// TODO: Find optimal Height/Width (0.1-0.3)
constexpr float kBoxEdgeSize = 0.2f; // Used to establish tracking box dimensions
constexpr float kUsToMs = 1000.0f; // Used to convert from microseconds to millis

// This calculator manages the regions being tracked for each individual sticker
// and adjusts the regions being tracked if a change is detected in a sticker's
// initial anchor placement. Regions being tracked that have no associated sticker
// will be automatically removed upon the next iteration of the graph to optimize
// performance and remove all sticker artifacts
//
// Input:
//  ANCHORS - Initial anchor data (tracks changes and where to re/position) [REQUIRED]
//  BOXES - Used in cycle, boxes being tracked meant to update positions [OPTIONAL
//  - provided by subgraph]
// Output:
//  START_POS - Positions of boxes being tracked (can be overwritten with ID) [REQUIRED]
//  CANCEL_ID - Single integer ID of tracking box to remove from tracker subgraph [OPTIONAL]
//  ANCHORS - Updated set of anchors with tracked and normalized X,Y,Z [REQUIRED]
//
// Example config:
// node {
//   calculator: "TrackedAnchorManagerCalculator"
//   input_stream: "ANCHORS:initial_anchor_data"
//   input_stream: "BOXES:boxes"
//   input_stream_info: {
//     tag_index: 'BOXES'
//     back_edge: true
//   }
//   output_stream: "START_POS:start_pos"
//   output_stream: "CANCEL_ID:cancel_object_id"
//   output_stream: "ANCHORS:tracked_scaled_anchor_data"
// }

class TrackedAnchorManagerCalculator : public CalculatorBase {
private:
  // Anchor data from the last iteration of the graph (Used to detect repositioning)
  std::vector<Anchor> previous_iteration_anchor_data_;

public:
  static ::mediapipe::Status GetContract(CalculatorContract* cc) {
    RET_CHECK(cc->Inputs().HasTag(kAnchorsTag));
    RET_CHECK(cc->Outputs().HasTag(kAnchorsTag)
      && cc->Outputs().HasTag(kBoxesOutputTag));

    cc->Inputs().Tag(kAnchorsTag).Set<std::vector<Anchor>>();

    if(cc->Inputs().HasTag(kBoxesInputTag)) {
      cc->Inputs().Tag(kBoxesInputTag).Set<TimedBoxProtoList>();
    }

    cc->Outputs().Tag(kAnchorsTag).Set<std::vector<Anchor>>();
    cc->Outputs().Tag(kBoxesOutputTag).Set<TimedBoxProtoList>();

    if(cc->Outputs().HasTag(kCancelTag)) {
      cc->Outputs().Tag(kCancelTag).Set<int>();
    }

    return ::mediapipe::OkStatus();
  }

  ::mediapipe::Status Open(CalculatorContext* cc) override {
    cc->SetOffset(TimestampDiff(0));
    return ::mediapipe::OkStatus();
  }

  ::mediapipe::Status Process(CalculatorContext* cc) override;
};
REGISTER_CALCULATOR(TrackedAnchorManagerCalculator);

::mediapipe::Status TrackedAnchorManagerCalculator::Process(CalculatorContext* cc) {
  std::vector<Anchor> current_anchor_data = cc->Inputs().Tag(kAnchorsTag).Get<std::vector<Anchor>>();
  mediapipe::Timestamp timestamp = cc->InputTimestamp();

  auto pos_boxes = absl::make_unique<TimedBoxProtoList>();
  std::vector<Anchor> tracked_scaled_anchor_data;

  for(Anchor anchor : current_anchor_data) {
    // Check if anchor is currently being tracked
    bool anchor_is_being_tracked = false;
    for(const TimedBoxProto& box : cc->Inputs().Tag(kBoxesInputTag).Get<TimedBoxProtoList>().box()) {
      if(box.id() == anchor.sticker_id) {
        anchor_is_being_tracked = true;
        break;
      }
    }
    // Check if initial anchor position has changed
    bool anchor_replaced = false;
    for(const Anchor &previous_anchor : previous_iteration_anchor_data_) {
      if(previous_anchor.sticker_id == anchor.sticker_id) {
        if(previous_anchor.x != anchor.x || previous_anchor.y != anchor.y) {
          anchor_replaced = true;
          break;
        }
      }
    }
    // If anchor is not being tracked
    if(!anchor_is_being_tracked || anchor_replaced) {
      // Delete all existing associated tracking boxes if they exist
      cc->Outputs().Tag(kCancelTag).AddPacket(MakePacket<int>(anchor.sticker_id).At(timestamp++));
      TimedBoxProto* box = pos_boxes->add_box();
      box->set_left(anchor.x - kBoxEdgeSize * 0.5f);
      box->set_right(anchor.x + kBoxEdgeSize * 0.5f);
      box->set_top(anchor.y - kBoxEdgeSize * 0.5f);
      box->set_bottom(anchor.y + kBoxEdgeSize * 0.5f);
      box->set_id(anchor.sticker_id);
      box->set_time_msec((timestamp++).Microseconds() / kUsToMs);
      // Default value for normalized z (scale factor)
      anchor.z = 1.0;
    }
    // If anchor is being tracked and was not replaced or repositioned
    else if (cc->Inputs().HasTag(kBoxesInputTag)) {
      const TimedBoxProtoList box_list = cc->Inputs().Tag(kBoxesInputTag).Get<TimedBoxProtoList>();
      for(const auto& box : box_list.box())
      {
        if(box.id() == anchor.sticker_id) {
          // Get center x normalized coordinate [0.0-1.0]
          anchor.x = (box.left() + box.right()) * 0.5f;

          // Get center y normalized coordinate [0.0-1.0]
          anchor.y = (box.top() + box.bottom()) * 0.5f;

          // Get center z coordinate [z starts at normalized 1.0 and scales inversely with box-width]
          // TODO: Look into issues with uniform scaling on x-axis and y-axis
          anchor.z = kBoxEdgeSize / (box.right() - box.left());
        }
      }
    }
    tracked_scaled_anchor_data.emplace_back(anchor);
  }

  // Set previous anchors for next iteration
  previous_iteration_anchor_data_ = current_anchor_data;

  cc->Outputs().Tag(kAnchorsTag).AddPacket(MakePacket<std::vector<Anchor>>(tracked_scaled_anchor_data).At(cc->InputTimestamp()));
  cc->Outputs().Tag(kBoxesOutputTag).Add(pos_boxes.release(), timestamp++);

  return ::mediapipe::OkStatus();
}
}
