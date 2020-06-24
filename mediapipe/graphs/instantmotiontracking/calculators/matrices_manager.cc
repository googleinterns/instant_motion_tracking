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
#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/port/ret_check.h"
#include "mediapipe/framework/port/status.h"
#include "mediapipe/graphs/object_detection_3d/calculators/box.h"
#include "mediapipe/graphs/object_detection_3d/calculators/model_matrix.pb.h"
#include "mediapipe/graphs/instantmotiontracking/calculators/transformations.h"

namespace mediapipe {

using Matrix4fRM = Eigen::Matrix<float, 4, 4, Eigen::RowMajor>;

constexpr char kFinalTranslationsTag[] = "TRANSLATION_DATA";
constexpr char kIMUDataTag[] = "IMU_DATA";
constexpr char kUserRotationsTag[] = "USER_ROTATIONS";
constexpr char kModelMatricesTag[] = "MODEL_MATRICES";


// Intermediary for rotation and translation data to model matrix usable by
// gl_animation_overlay_calculator
//
// Input:
//  IMU_DATA - float[3] of [roll, pitch, yaw] of device
//  USER_ROTATIONS - UserRotations with corresponding radians of rotation
//  TRANSLATION_DATA - All stickers final translation data (vector of
//  Translation objects)
// Output:
//  MODEL_MATRICES - TimedModelMatrixProtoList of all objects to render
//
// Example config:
// node{
//  calculator: "MatricesManagerCalculator"
//  input_stream: "IMU_DATA:imu_data"
//  input_stream: "USER_ROTATIONS:user_rotation_data"
//  input_stream: "TRANSLATION_DATA:final_translation_data"
//  output_stream: "MODEL_MATRICES:model_matrices"
// }

class MatricesManagerCalculator : public CalculatorBase {
 public:
  static ::mediapipe::Status GetContract(CalculatorContract* cc);
  ::mediapipe::Status Open(CalculatorContext* cc) override;
  ::mediapipe::Status Process(CalculatorContext* cc) override;
  Matrix4fRM generateEigenModelMatrix(Eigen::Vector3f translation_vector,
    Eigen::Matrix3f rotation_submatrix);
  Eigen::Vector3f generateTranslationVector(Translation translation);
  Eigen::Matrix3f generateRotationSubmatrix(float roll, float pitch, float yaw, float user_rotation);
};

REGISTER_CALCULATOR(MatricesManagerCalculator);

::mediapipe::Status MatricesManagerCalculator::GetContract(
    CalculatorContract* cc) {
  RET_CHECK(!cc->Inputs().GetTags().empty());
  RET_CHECK(!cc->Outputs().GetTags().empty());

  if (cc->Inputs().HasTag(kFinalTranslationsTag)) {
    cc->Inputs().Tag(kFinalTranslationsTag).Set<std::vector<Translation>>();
  }
  if (cc->Inputs().HasTag(kIMUDataTag)) {
    cc->Inputs().Tag(kIMUDataTag).Set<float[]>();
  }
  if (cc->Inputs().HasTag(kUserRotationsTag)) {
    cc->Inputs().Tag(kUserRotationsTag).Set<std::vector<UserRotation>>();
  }
  if (cc->Outputs().HasTag(kModelMatricesTag)) {
    cc->Outputs().Tag(kModelMatricesTag).Set<TimedModelMatrixProtoList>();
  }

  return ::mediapipe::OkStatus();
}

::mediapipe::Status MatricesManagerCalculator::Open(CalculatorContext* cc) {
  cc->SetOffset(TimestampDiff(0));
  return ::mediapipe::OkStatus();
}

::mediapipe::Status MatricesManagerCalculator::Process(CalculatorContext* cc) {
  auto model_matrices = std::make_unique<TimedModelMatrixProtoList>();
  TimedModelMatrixProtoList* model_matrix_list = model_matrices.get();
  model_matrix_list->clear_model_matrix();

  const std::vector<UserRotation> user_rotation_data =
      cc->Inputs().Tag(kUserRotationsTag).Get<std::vector<UserRotation>>();

  const std::vector<Translation> translation_data =
      cc->Inputs()
          .Tag(kFinalTranslationsTag)
          .Get<std::vector<Translation>>();

  // Device IMU Data definitions
  const float roll = cc->Inputs().Tag(kIMUDataTag).Get<float[]>()[0];
  const float pitch = cc->Inputs().Tag(kIMUDataTag).Get<float[]>()[1];
  const float yaw = cc->Inputs().Tag(kIMUDataTag).Get<float[]>()[2];

  for (UserRotation user_rotation : user_rotation_data) {
    for (Translation translation : translation_data) {
      if (user_rotation.sticker_id == translation.sticker_id) {
        TimedModelMatrixProto* model_matrix =
            model_matrix_list->add_model_matrix();
        model_matrix->set_id(user_rotation.sticker_id);

        Eigen::Vector3f translation_vector = generateTranslationVector(translation);
        Eigen::Matrix3f rotation_submatrix = generateRotationSubmatrix(roll, pitch, yaw, user_rotation.radians);

        Matrix4fRM mvp_matrix = generateEigenModelMatrix(translation_vector, rotation_submatrix);

        // The generated model matrix must be mapped to TimedModelMatrixProto
        // (col-wise)
        for (int i = 0; i < mvp_matrix.rows(); ++i) {
          for (int j = 0; j < mvp_matrix.cols(); ++j) {
            model_matrix->add_matrix_entries(mvp_matrix(j, i));
          }
        }
        break;
      }
    }
  }

  cc->Outputs()
      .Tag(kModelMatricesTag)
      .Add(model_matrices.release(), cc->InputTimestamp());

  return ::mediapipe::OkStatus();
}

// Using the tracked translation data, generate a vector for MVP translation
Eigen::Vector3f MatricesManagerCalculator::generateTranslationVector(Translation translation) {
  Eigen::Vector3f t_vector(translation.x, translation.y, translation.z);
  return t_vector;
}

// Generate the submatrix defining rotation using IMU data and user rotations
Eigen::Matrix3f MatricesManagerCalculator::generateRotationSubmatrix(float roll, float pitch, float yaw, float user_rotation_radians) {
  Eigen::Matrix3f r_submatrix;
  r_submatrix =
      Eigen::AngleAxisf(yaw - user_rotation_radians, Eigen::Vector3f::UnitY()) *
      Eigen::AngleAxisf(pitch, Eigen::Vector3f::UnitZ()) *
      Eigen::AngleAxisf(roll - (M_PI / 2), Eigen::Vector3f::UnitX());
  return r_submatrix;
}

// Generates a model matrix via Eigen with appropriate transformations
Matrix4fRM MatricesManagerCalculator::generateEigenModelMatrix(
    Eigen::Vector3f translation_vector, Eigen::Matrix3f rotation_submatrix) {
  // Define basic empty model matrix
  Matrix4fRM mvp_matrix;
  mvp_matrix << 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
      0.0, 0.0, 1.0;  // trailing 1.0 required by OpenGL

  // Set the translation vector
  mvp_matrix.bottomLeftCorner<1, 3>() = translation_vector;

  // Set the rotation submatrix
  mvp_matrix.topLeftCorner<3, 3>() = rotation_submatrix;

  return mvp_matrix;
}
}
