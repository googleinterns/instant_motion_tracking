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

#include <memory>
#include <cmath>
#include "Eigen/Dense"
#include "Eigen/src/Core/util/Constants.h"
#include "Eigen/src/Geometry/Quaternion.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/detection.pb.h"
#include "mediapipe/framework/formats/location_data.pb.h"
#include "mediapipe/framework/port/status.h"
#include "mediapipe/framework/port/ret_check.h"
#include "mediapipe/util/tracking/box_tracker.h"
#include "mediapipe/graphs/instantmotiontracking/calculators/transformations.h"

namespace mediapipe {
using Matrix4fCM = Eigen::Matrix<float, 4, 4, Eigen::ColMajor>;
using Vector3f = Eigen::Vector3f;
using Matrix3f = Eigen::Matrix3f;
constexpr char kIMUMatrixTag[] = "IMU_ROTATION";
constexpr char kSentinelTag[] = "SENTINEL";
constexpr char kAnchorsTag[] = "ANCHORS";
constexpr char kBoxesInputTag[] = "BOXES";
constexpr char kBoxesOutputTag[] = "START_POS";
constexpr char kMatricesTag[] = "MATRICES";
constexpr char kCancelTag[] = "CANCEL_ID";
constexpr char kFOVSidePacketTag[] = "FOV";
constexpr char kAspectRatioSidePacketTag[] = "ASPECT_RATIO";
// TODO: Find optimal Height/Width (0.1-0.3)
// Used to establish tracking box dimensions
constexpr float kBoxEdgeSize = 0.2f;
// Used to convert from microseconds to millis
constexpr float kUsToMs = 1000.0f;
// initial Z value (-10 is center point in visual range for OpenGL render)
constexpr float kInitialZ = -10.0f;
// Device properties that will be preset by side packets
float vertical_fov_radians_ = 0.0f;
float aspect_ratio_ = 0.0f;

// and adjusts the regions being tracked if a change is detected in a sticker's
// initial anchor placement. Regions being tracked that have no associated sticker
// will be automatically removed upon the next iteration of the graph to optimize
// performance and remove all sticker artifacts
//
// Input Side Packets:
//  FOV - Vertical field of view for device [REQUIRED - Defines perspective matrix]
//  ASPECT_RATIO - Aspect ratio of device [REQUIRED - Defines perspective matrix]
//
// Input:
//  IMU_ROTATION - float[9] of row-major device rotation matrix [REQUIRED]
//  SENTINEL - ID of sticker which has an anchor that must be reset (-1 when no
// anchor must be reset) [REQUIRED]
//  ANCHORS - Initial anchor data (tracks changes and where to re/position) [REQUIRED]
//  BOXES - Used in cycle, boxes being tracked meant to update positions [OPTIONAL
//  - provided by subgraph]
// Output:
//  START_POS - Positions of boxes being tracked (can be overwritten with ID) [REQUIRED]
//  CANCEL_ID - Single integer ID of tracking box to remove from tracker subgraph [OPTIONAL]
//  MATRICES - Model matrices with rotation and translation [REQUIRED]
//
// Example config:
// node {
//   calculator: "TrackedAnchorManagerCalculator"
//   input_stream: "IMU_ROTATION:imu_rotation_matrix"
//   input_stream: "SENTINEL:sticker_sentinel"
//   input_stream: "ANCHORS:initial_anchor_data"
//   input_stream: "BOXES:boxes"
//   input_stream_info: {
//     tag_index: 'BOXES'
//     back_edge: true
//   }
//   output_stream: "START_POS:start_pos"
//   output_stream: "CANCEL_ID:cancel_object_id"
//   output_stream: "MATRICES:model_matrices"
//  input_side_packet: "FOV:vertical_fov_radians"
//  input_side_packet: "ASPECT_RATIO:aspect_ratio"
// }

class TrackedAnchorManagerCalculator : public CalculatorBase {
private:
  // Previous graph iteration anchor data
  std::vector<Anchor> previous_anchor_data;

  // Generates a model matrix via Eigen with appropriate transformations
  const Matrix4fCM GenerateEigenModelMatrix(
      Vector3f translation_vector, Matrix3f rotation_submatrix) {
    // Define basic empty model matrix
    Matrix4fCM mvp_matrix;

    // Set the translation vector
    mvp_matrix.topRightCorner<3, 1>() = translation_vector;

    // Set the rotation submatrix
    mvp_matrix.topLeftCorner<3, 3>() = rotation_submatrix;

    // Set trailing 1.0 required by OpenGL to define coordinate space
    mvp_matrix(3,3) = 1.0f;

    return mvp_matrix;
  }

  // TODO: Investigate possible differences in warping of tracking speed across screen
  // Using the sticker anchor data, a translation vector can be generated in OpenGL coordinate space
  const Vector3f GenerateAnchorVector(const Anchor tracked_anchor) {
    // Using an initial z-value in OpenGL space, generate a new base z-axis value to mimic scaling by distance.
    const float z = kInitialZ * tracked_anchor.z;

    // Using triangle geometry, the minimum for a y-coordinate that will appear in the view field
    // for the given z value above can be found.
    const float y_half_range = z * (tan(vertical_fov_radians_ * 0.5f));

    // The aspect ratio of the device and y_minimum calculated above can be used to find the
    // minimum value for x that will appear in the view field of the device screen.
    const float x_half_range = y_half_range * aspect_ratio_;

    // Given the minimum bounds of the screen in OpenGL space, the tracked anchor coordinates
    // can be converted to OpenGL coordinate space.
    //
    // (i.e: X and Y will be converted from [0.0-1.0] space to [x_minimum, -x_minimum] space
    // and [y_minimum, -y_minimum] space respectively)
    const float x = (-2.0f * tracked_anchor.x * x_half_range) + x_half_range;
    const float y = (-2.0f * tracked_anchor.y * y_half_range) + y_half_range;

    // A translation transformation vector can be generated via Eigen
    const Vector3f t_vector(x,y,z);
    return t_vector;
  }

public:
  static ::mediapipe::Status GetContract(CalculatorContract* cc) {
    RET_CHECK(cc->Inputs().HasTag(kAnchorsTag)
      && cc->Inputs().HasTag(kSentinelTag)
      && cc->Inputs().HasTag(kIMUMatrixTag)
      && cc->InputSidePackets().HasTag(kFOVSidePacketTag)
      && cc->InputSidePackets().HasTag(kAspectRatioSidePacketTag));
    RET_CHECK(cc->Outputs().HasTag(kBoxesOutputTag)
      && cc->Outputs().HasTag(kMatricesTag));

    cc->Inputs().Tag(kAnchorsTag).Set<std::vector<Anchor>>();
    cc->Inputs().Tag(kSentinelTag).Set<int>();
    cc->Inputs().Tag(kIMUMatrixTag).Set<float[]>();

    if (cc->Inputs().HasTag(kBoxesInputTag)) {
      cc->Inputs().Tag(kBoxesInputTag).Set<TimedBoxProtoList>();
    }

    cc->InputSidePackets().Tag(kFOVSidePacketTag).Set<float>();
    cc->InputSidePackets().Tag(kAspectRatioSidePacketTag).Set<float>();

    cc->Outputs().Tag(kMatricesTag).Set<std::vector<Matrix4fCM>>();
    cc->Outputs().Tag(kBoxesOutputTag).Set<TimedBoxProtoList>();

    if (cc->Outputs().HasTag(kCancelTag)) {
      cc->Outputs().Tag(kCancelTag).Set<int>();
    }

    return ::mediapipe::OkStatus();
  }

  ::mediapipe::Status Open(CalculatorContext* cc) override {
    // Set device properties from side packets
    vertical_fov_radians_ = cc->InputSidePackets().Tag(kFOVSidePacketTag).Get<float>();
    aspect_ratio_ = cc->InputSidePackets().Tag(kAspectRatioSidePacketTag).Get<float>();
    return ::mediapipe::OkStatus();
  }

  ::mediapipe::Status Process(CalculatorContext* cc) override;
};
REGISTER_CALCULATOR(TrackedAnchorManagerCalculator);

::mediapipe::Status TrackedAnchorManagerCalculator::Process(CalculatorContext* cc) {
  mediapipe::Timestamp timestamp = cc->InputTimestamp();
  const int sticker_sentinel = cc->Inputs().Tag(kSentinelTag).Get<int>();
  std::vector<Anchor> current_anchor_data = cc->Inputs().Tag(kAnchorsTag).Get<std::vector<Anchor>>();
  auto pos_boxes = absl::make_unique<TimedBoxProtoList>();
  std::vector<Anchor> tracked_scaled_anchor_data;
  std::vector<Matrix4fCM> final_matrices_data;

  // Device IMU rotation submatrix
  const auto imu_matrix = cc->Inputs().Tag(kIMUMatrixTag).Get<float[]>();
  Matrix3f imu_rotation_submatrix;
  int idx = 0;
  for (int x = 0; x < 3; x++) {
    for (int y = 0; y < 3; y++) {
      // Input matrix is row-major matrix, it must be reformatted to column-major
      // via transpose procedure
      imu_rotation_submatrix(y, x) = imu_matrix[idx++];
    }
  }

  // Delete any boxes being tracked without an associated anchor
  for (const TimedBoxProto& box : cc->Inputs().Tag(kBoxesInputTag)
       .Get<TimedBoxProtoList>().box()) {
    bool anchor_exists = false;
    for (Anchor anchor : current_anchor_data) {
      if (box.id() == anchor.sticker_id) {
        anchor_exists = true;
        break;
      }
    }
    if (!anchor_exists) {
      cc->Outputs().Tag(kCancelTag).AddPacket(MakePacket<int>(box.id()).At(timestamp++));
    }
  }

  // Perform tracking or updating for each anchor position
  for (Anchor anchor : current_anchor_data) {
    // Check if anchor position is being reset by user in this graph iteration
    if (sticker_sentinel == anchor.sticker_id) {
      // Delete associated tracking box
      // TODO: BoxTrackingSubgraph should accept vector to avoid breaking timestamp rules
      cc->Outputs().Tag(kCancelTag).AddPacket(MakePacket<int>(anchor.sticker_id).At(timestamp++));
      // Add a tracking box
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
    // Anchor position was not reset by user
    else {
      // Attempt to update anchor position from tracking subgraph (TimedBoxProto)
      bool updated_from_tracker = false;
      const TimedBoxProtoList box_list = cc->Inputs().Tag(kBoxesInputTag).Get<TimedBoxProtoList>();
      for (const auto& box : box_list.box())
      {
        if (box.id() == anchor.sticker_id) {
          // Get center x normalized coordinate [0.0-1.0]
          anchor.x = (box.left() + box.right()) * 0.5f;
          // Get center y normalized coordinate [0.0-1.0]
          anchor.y = (box.top() + box.bottom()) * 0.5f;
          // Get center z coordinate [z starts at normalized 1.0 and scales
          // inversely with box-width]
          // TODO: Look into issues with uniform scaling on x-axis and y-axis
          anchor.z = kBoxEdgeSize / (box.right() - box.left());
          updated_from_tracker = true;
          break;
        }
      }
      // If anchor position was not updated from tracker, create new tracking box
      // at last recorded anchor coordinates. This will allow all current stickers
      // to be tracked at approximately last location even if re-acquisitioning
      // in the BoxTrackingSubgraph encounters errors
      if (!updated_from_tracker) {
        for (Anchor prev_anchor : previous_anchor_data) {
          if (anchor.sticker_id == prev_anchor.sticker_id) {
            anchor = prev_anchor;
            TimedBoxProto* box = pos_boxes->add_box();
            box->set_left(anchor.x - kBoxEdgeSize * 0.5f);
            box->set_right(anchor.x + kBoxEdgeSize * 0.5f);
            box->set_top(anchor.y - kBoxEdgeSize * 0.5f);
            box->set_bottom(anchor.y + kBoxEdgeSize * 0.5f);
            box->set_id(anchor.sticker_id);
            box->set_time_msec((timestamp++).Microseconds() / kUsToMs);
            // Default value for normalized z (scale factor)
            anchor.z = 1.0;
            break;
          }
        }
      }
    }
    tracked_scaled_anchor_data.emplace_back(anchor);
    // A vector representative of the translation of the object in OpenGL coordinate space must be generated
    const Vector3f translation_vector = GenerateAnchorVector(anchor);
    Matrix4fCM matrix = GenerateEigenModelMatrix(translation_vector, imu_rotation_submatrix);
    final_matrices_data.emplace_back(matrix);
  }
  // Set anchor data for next iteration
  previous_anchor_data = tracked_scaled_anchor_data;

  cc->Outputs().Tag(kMatricesTag).AddPacket(MakePacket<std::vector<Matrix4fCM>>(final_matrices_data).At(cc->InputTimestamp()));
  cc->Outputs().Tag(kBoxesOutputTag).Add(pos_boxes.release(), cc->InputTimestamp());

  return ::mediapipe::OkStatus();
}
}
