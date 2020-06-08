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

#include "Eigen/Dense"
#include "Eigen/src/Core/util/Constants.h"
#include "Eigen/src/Geometry/Quaternion.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/port/ret_check.h"
#include "mediapipe/framework/port/status.h"
#include "mediapipe/graphs/object_detection_3d/calculators/box.h"
#include "mediapipe/graphs/object_detection_3d/calculators/model_matrix.pb.h"
#include <cmath>
#include "sticker.h"

namespace mediapipe {
  namespace{
    using Matrix4fRM = Eigen::Matrix<float, 4, 4, Eigen::RowMajor>;
  }

// Intermediary for sticker data to model matrix usable by gl_animation_overlay_calculator
// *The structure of process() is largely dependent on the rendering system format
//
// Input:
//  STICKERS - Vector of Sticker objects to determine rendering
//  IMU_DATA - float array of radians [roll, pitch, yaw] from device orientation sensors
// Output:
//  MODEL_MATRIX_FLOAT - Result model matrix, in float array form
//
// Example config:
//node{
//  calculator: "ModelMatrixManagerCalculator"
//  input_stream: "STICKERS:sticker_data"
//  input_stream: "IMU_DATA:imu_data"
//  output_stream: "MODEL_MATRIX_FLOAT:model_matrix"
//}
//

class ModelMatrixManagerCalculator : public CalculatorBase {
 public:
  ModelMatrixManagerCalculator() {}
  ~ModelMatrixManagerCalculator() override {}
  ModelMatrixManagerCalculator(
      const ModelMatrixManagerCalculator&) = delete;
  ModelMatrixManagerCalculator& operator=(
      const ModelMatrixManagerCalculator&) = delete;

  static ::mediapipe::Status GetContract(CalculatorContract* cc);

  ::mediapipe::Status Open(CalculatorContext* cc) override;

  ::mediapipe::Status Process(CalculatorContext* cc) override;

  void setObjectRotation(float roll, float pitch, float yaw, float userRotation);
  void setObjectTranslation(float userScaling);

 private:
   Matrix4fRM matrix_model;
   float model_matrix_float_array[16] = {0.83704215,  -0.36174262, 0.41049102, 0.0,
                                     0.06146407,  0.8076706,   0.5864218,  0.0,
                                     -0.54367524, -0.4656292,  0.69828844, 0.0,
                                     0.0,         0.0,         -98.64117,  1.0};
};

REGISTER_CALCULATOR(ModelMatrixManagerCalculator);

::mediapipe::Status ModelMatrixManagerCalculator::GetContract(CalculatorContract* cc) {
      RET_CHECK(!cc->Inputs().GetTags().empty());
      RET_CHECK(!cc->Outputs().GetTags().empty());

      if (cc->Inputs().HasTag("STICKERS"))
        cc->Inputs().Tag("STICKERS").Set<std::vector<Sticker>>();

      if (cc->Inputs().HasTag("IMU_DATA"))
        cc->Inputs().Tag("IMU_DATA").Set<float[]>();

      if (cc->Outputs().HasTag("MODEL_MATRIX_FLOAT"))
        cc->Outputs().Tag("MODEL_MATRIX_FLOAT").Set<std::array<float, 16>>();

  return ::mediapipe::OkStatus();
}

::mediapipe::Status ModelMatrixManagerCalculator::Open(CalculatorContext* cc) {
  cc->SetOffset(TimestampDiff(0));

  // Initialize baseline matrix
  matrix_model << 0.83704215,  -0.36174262, 0.41049102,0.0,
                  0.06146407,  0.8076706,   0.5864218, 0.0,
                  -0.54367524, -0.4656292,  0.69828844, 0.0,
                  0.0,         0.0,         -98.64117,  1.0;

  return ::mediapipe::OkStatus();
}

::mediapipe::Status ModelMatrixManagerCalculator::Process(
    CalculatorContext* cc) {
      // Device IMU Data definitions
      float roll = cc->Inputs().Tag("IMU_DATA").Get<float[]>()[0];
      float pitch = cc->Inputs().Tag("IMU_DATA").Get<float[]>()[1];
      float yaw = cc->Inputs().Tag("IMU_DATA").Get<float[]>()[2];

      // The current implementation of UI only uses one sticker, so the values
      // for userRotation, userScaling, and scalingRatio (from tracking), will be
      // based on the LATEST sticker in the sticker vector input
      for(Sticker sticker:cc->Inputs().Tag("STICKERS").Get<std::vector<Sticker>>()) {
        setObjectRotation(roll, pitch, yaw, sticker.userRotation);
        setObjectTranslation(sticker.userScaling);
      }

      // Create deep copy from Eigen matrix to float array
      auto model_matrix = absl::make_unique<std::array<float, 16>>();
      int i = 0;
      for (int x = 0; x<4;x++) {
        for (int y = 0; y<4;y++) {
          (*model_matrix.get())[i] = matrix_model(x,y);
          i++;
        }
      }

    cc->Outputs()
        .Tag("MODEL_MATRIX_FLOAT")
        .Add(model_matrix.release(), cc->InputTimestamp());

  return ::mediapipe::OkStatus();
}

void ModelMatrixManagerCalculator::setObjectRotation(float roll, float pitch, float yaw, float userRotation) {
  Eigen::Matrix3f r;
  r = Eigen::AngleAxisf(-pitch, Eigen::Vector3f::UnitY()) *
      Eigen::AngleAxisf(userRotation - yaw, Eigen::Vector3f::UnitZ()) *
      Eigen::AngleAxisf(roll, Eigen::Vector3f::UnitX());
  matrix_model.topLeftCorner<3, 3>() = r;
}

void ModelMatrixManagerCalculator::setObjectTranslation(float userScaling) {
  matrix_model(3,2) = (1.0 * -98.64117) + userScaling; //stickerRatio
}

}  // namespace mediapipe
