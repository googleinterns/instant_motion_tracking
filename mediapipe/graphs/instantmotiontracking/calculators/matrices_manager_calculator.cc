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

namespace {
  using Matrix4fRM = Eigen::Matrix<float, 4, 4, Eigen::RowMajor>;
  constexpr char kAnchorsTag[] = "ANCHORS";
  constexpr char kIMUDataTag[] = "IMU_DATA";
  constexpr char kUserRotationsTag[] = "USER_ROTATIONS";
  constexpr char kUserScalingsTag[] = "USER_SCALINGS";
  constexpr char kModelMatricesTag[] = "MODEL_MATRICES";
  // (68 degrees, 4:3 for Pixel 4)
  const float vertical_fov_radians_ = (68.0f) * M_PI / 180.0f;
  const float aspect_ratio_ = (4.0f / 3.0f);
  // initial Z value (-98 is just in visual range for OpenGL render)
  const float initial_z_ = -98;
}

// Intermediary for rotation and translation data to model matrix usable by
// gl_animation_overlay_calculator
//
// All inputs are required for intended functionality (scaling, rotations, etc.)
// however, no inputs are required to run without errors
//
// Input:
//  ANCHORS - Anchor data with normalized x,y,z coordinates [REQUIRED]
//  IMU_DATA - float[3] of [roll, pitch, yaw] of device [REQUIRED]
//  USER_ROTATIONS - UserRotations with corresponding radians of rotation [REQUIRED]
//  TRANSLATION_DATA - All stickers final translation data (vector of
//  Translation objects) [REQUIRED]
// Output:
//  MODEL_MATRICES - TimedModelMatrixProtoList of all objects to render [REQUIRED]
//
// Example config:
// node{
//  calculator: "MatricesManagerCalculator"
//  input_stream: "ANCHORS:tracked_scaled_anchor_data"
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
  private:
    Matrix4fRM generateEigenModelMatrix(const Eigen::Vector3f translation_vector,
      const Eigen::Matrix3f rotation_submatrix);
    Eigen::Vector3f generateTranslationVector(const Anchor tracked_anchor, const float user_scaling_increment);
    Eigen::Matrix3f generateRotationSubmatrix(const float roll, const float pitch, const float yaw, const float user_rotation_radians);
    // TODO: Adjust lookup function if total number of stickers is uncapped to improve performance
    float getUserScaling(std::vector<UserScaling> scalings, int sticker_id) {
      for (const UserScaling &user_scaling : scalings) {
        if (user_scaling.sticker_id == sticker_id) {
          return user_scaling.scaling_increment;
        }
      }
    }
    float getUserRotation(std::vector<UserRotation> rotations, int sticker_id) {
      for (const UserRotation &rotation : rotations) {
        if (rotation.sticker_id == sticker_id) {
          return rotation.radians;
        }
      }
    }
};

REGISTER_CALCULATOR(MatricesManagerCalculator);

::mediapipe::Status MatricesManagerCalculator::GetContract(
    CalculatorContract* cc) {
  RET_CHECK(cc->Inputs().HasTag(kAnchorsTag)
    && cc->Inputs().HasTag(kIMUDataTag)
    && cc->Inputs().HasTag(kUserRotationsTag)
    && cc->Inputs().HasTag(kUserScalingsTag));
  RET_CHECK(cc->Outputs().HasTag(kModelMatricesTag));

  if (cc->Inputs().HasTag(kAnchorsTag)) {
    cc->Inputs().Tag(kAnchorsTag).Set<std::vector<Anchor>>();
  }
  if (cc->Inputs().HasTag(kIMUDataTag)) {
    cc->Inputs().Tag(kIMUDataTag).Set<float[]>();
  }
  if (cc->Inputs().HasTag(kUserScalingsTag)) {
      cc->Inputs().Tag(kUserScalingsTag).Set<std::vector<UserScaling>>();
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

  const std::vector<UserScaling> user_scaling_data =
        cc->Inputs().Tag(kUserScalingsTag).Get<std::vector<UserScaling>>();

  const std::vector<Anchor> translation_data =
      cc->Inputs()
          .Tag(kAnchorsTag)
          .Get<std::vector<Anchor>>();

  // Device IMU Data definitions
  const float roll = cc->Inputs().Tag(kIMUDataTag).Get<float[]>()[0];
  const float pitch = cc->Inputs().Tag(kIMUDataTag).Get<float[]>()[1];
  const float yaw = cc->Inputs().Tag(kIMUDataTag).Get<float[]>()[2];

  for (const Anchor &anchor : translation_data) {
    int id = anchor.sticker_id;

    TimedModelMatrixProto* model_matrix =
        model_matrix_list->add_model_matrix();
    model_matrix->set_id(id);

    float rotation = getUserRotation(user_rotation_data, id);
    float scaling = getUserScaling(user_scaling_data, id);

    const Eigen::Vector3f translation_vector = generateTranslationVector(anchor, scaling);
    const Eigen::Matrix3f rotation_submatrix = generateRotationSubmatrix(roll, pitch, yaw, rotation);

    Matrix4fRM mvp_matrix = generateEigenModelMatrix(translation_vector, rotation_submatrix);

    // The generated model matrix must be mapped to TimedModelMatrixProto (col-wise)
    for (int i = 0; i < mvp_matrix.rows(); ++i) {
      for (int j = 0; j < mvp_matrix.cols(); ++j) {
        model_matrix->add_matrix_entries(mvp_matrix(j, i));
      }
    }
  }

  cc->Outputs()
      .Tag(kModelMatricesTag)
      .Add(model_matrices.release(), cc->InputTimestamp());

  return ::mediapipe::OkStatus();
}

// Using the tracked translation data, generate a vector for MVP translation
Eigen::Vector3f MatricesManagerCalculator::generateTranslationVector(Anchor tracked_anchor, float user_scaling_increment) {
  // Convert from normalized [0.0-1.0] to openGL on-screen coordinates
  float z = (initial_z_ + user_scaling_increment) * tracked_anchor.z;

  // TODO: Investigate possible differences in warping of tracking speed across screen
  float y_minimum = z * (tan(vertical_fov_radians_ /2));
  // Minimum y value appearing on screen at z distance
  float x_minimum = y_minimum * (1.0/aspect_ratio_);
  // Minimum x value appearing on screen at z distance

  float x = (tracked_anchor.x * (-x_minimum - x_minimum)) + x_minimum;
  float y = (tracked_anchor.y * (-y_minimum - y_minimum)) + y_minimum;

  Eigen::Vector3f t_vector(x,y,z);
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

  // Set the translation vector
  mvp_matrix.bottomLeftCorner<1, 3>() = translation_vector;

  // Set the rotation submatrix
  mvp_matrix.topLeftCorner<3, 3>() = rotation_submatrix;

  // Set trailing 1.0 required by OpenGL to define coordinate space
  mvp_matrix(3,3) = 1.0;

  return mvp_matrix;
}
}
