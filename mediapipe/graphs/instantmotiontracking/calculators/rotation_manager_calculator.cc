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
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/graphs/instantmotiontracking/calculators/transformations.h"

namespace mediapipe {

constexpr char kIMUDataTag[] = "IMU_DATA";
constexpr char kUserRotationsTag[] = "USER_ROTATIONS";
constexpr char kFinalRotationsTag[] = "ROTATION_DATA";

// Generates the final model matrix rotations via the IMU (orientation sensors)
// of the device and user rotation information
//
// Input:
//  IMU_DATA - float[3] of [roll, pitch, yaw] of device
//  USER_ROTATIONS - UserRotations with corresponding radians of rotation
// Output:
//  ROTATION_DATA - Combined rotational transformations for each sticker
//
// Example config:
// node {
//   calculator: "RotationManagerCalculator"
//  input_stream: "IMU_DATA:imu_data"
//  input_stream: "USER_ROTATIONS:user_rotation_data"
//  output_stream: "ROTATION_DATA:final_rotation_data"
// }

class RotationManagerCalculator : public CalculatorBase {
 public:
  static ::mediapipe::Status GetContract(CalculatorContract* cc) {
    RET_CHECK(!cc -> Inputs().GetTags().empty());
    RET_CHECK(!cc -> Outputs().GetTags().empty());

    if (cc -> Inputs().HasTag(kIMUDataTag))
      cc -> Inputs().Tag(kIMUDataTag).Set<float[]>();
    if (cc -> Inputs().HasTag(kUserRotationsTag))
      cc -> Inputs().Tag(kUserRotationsTag).Set<std::vector<UserRotation>>();
    if (cc -> Outputs().HasTag(kFinalRotationsTag))
      cc -> Outputs().Tag(kFinalRotationsTag).Set<std::vector<Rotation>>();

    return ::mediapipe::OkStatus();
  }

  ::mediapipe::Status Open(CalculatorContext* cc) final {
    cc -> SetOffset(TimestampDiff(0));
    return ::mediapipe::OkStatus();
  }

  ::mediapipe::Status Process(CalculatorContext* cc) final {
    std::vector<Rotation> combined_rotation_data;

    const std::vector<UserRotation> user_rotation_data =
        cc -> Inputs().Tag(kUserRotationsTag).Get<std::vector<UserRotation>>();

    // Device IMU Data definitions
    const float roll = cc -> Inputs().Tag(kIMUDataTag).Get<float[]>()[0];
    const float pitch = cc -> Inputs().Tag(kIMUDataTag).Get<float[]>()[1];
    const float yaw = cc -> Inputs().Tag(kIMUDataTag).Get<float[]>()[2];

    for (UserRotation user_rotation : user_rotation_data) {
      Rotation combined_rotation;
      // Default render must be rotated upright
      combined_rotation.x_radians = roll - (M_PI / 2);
      combined_rotation.y_radians = yaw - user_rotation.radians;
      combined_rotation.z_radians = pitch;
      combined_rotation.sticker_id = user_rotation.sticker_id;
      combined_rotation_data.emplace_back(combined_rotation);
    }

    if (cc -> Outputs().HasTag(kFinalRotationsTag))
      cc -> Outputs()
          .Tag(kFinalRotationsTag)
          .AddPacket(MakePacket<std::vector<Rotation>>(combined_rotation_data)
                         .At(cc -> InputTimestamp()));

    return ::mediapipe::OkStatus();
  }

  ::mediapipe::Status Close(CalculatorContext* cc) final {
    return ::mediapipe::OkStatus();
  }
};

REGISTER_CALCULATOR(RotationManagerCalculator);
}
