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
constexpr char kFinalRotationsTag[] = "ROTATION_DATA";
constexpr char kModelMatricesTag[] = "MODEL_MATRICES";

// Intermediary for rotation and translation data to model matrix usable by
// gl_animation_overlay_calculator
//
// Input:
//  ROTATION_DATA - All stickers final rotation data (vector of Rotation
//  objects)
//  TRANSLATION_DATA - All stickers final translation data (vector of
//  Translation objects)
// Output:
//  MODEL_MATRICES - TimedModelMatrixProtoList of all objects to render
//
// Example config:
// node{
//  calculator: "MatricesManagerCalculator"
//  input_stream: "TRANSLATION_DATA:final_translation_data"
//  input_stream: "ROTATION_DATA:final_rotation_data"
//  output_stream: "MODEL_MATRICES:model_matrices"
// }

class MatricesManagerCalculator : public CalculatorBase {
 public:
  static ::mediapipe::Status GetContract(CalculatorContract* cc);
  ::mediapipe::Status Open(CalculatorContext* cc) override;
  ::mediapipe::Status Process(CalculatorContext* cc) override;
  Matrix4fRM generateEigenModelMatrix(Rotation rotation,
                                      Translation translation);
};

REGISTER_CALCULATOR(MatricesManagerCalculator);

::mediapipe::Status MatricesManagerCalculator::GetContract(
    CalculatorContract* cc) {
  RET_CHECK(!cc -> Inputs().GetTags().empty());
  RET_CHECK(!cc -> Outputs().GetTags().empty());

  if (cc -> Inputs().HasTag(kFinalTranslationsTag))
    cc -> Inputs().Tag(kFinalTranslationsTag).Set<std::vector<Translation>>();
  if (cc -> Inputs().HasTag(kFinalRotationsTag))
    cc -> Inputs().Tag(kFinalRotationsTag).Set<std::vector<Rotation>>();
  if (cc -> Outputs().HasTag(kModelMatricesTag))
    cc -> Outputs().Tag(kModelMatricesTag).Set<TimedModelMatrixProtoList>();

  return ::mediapipe::OkStatus();
}

::mediapipe::Status MatricesManagerCalculator::Open(CalculatorContext* cc) {
  cc -> SetOffset(TimestampDiff(0));
  return ::mediapipe::OkStatus();
}

::mediapipe::Status MatricesManagerCalculator::Process(CalculatorContext* cc) {
  auto model_matrices = std::make_unique<TimedModelMatrixProtoList>();
  TimedModelMatrixProtoList* model_matrix_list = model_matrices.get();
  model_matrix_list -> clear_model_matrix();

  const std::vector<Rotation> rotation_data =
      cc -> Inputs()
          .Tag(kFinalRotationsTag)
          .Get<std::vector<Rotation>>();
  const std::vector<Translation> translation_data =
      cc -> Inputs()
          .Tag(kFinalTranslationsTag)
          .Get<std::vector<Translation>>();

  for (Rotation rotation : rotation_data) {
    for (Translation translation : translation_data) {
      if (rotation.sticker_id == translation.sticker_id) {
        TimedModelMatrixProto* model_matrix =
            model_matrix_list -> add_model_matrix();
        model_matrix -> set_id(rotation.sticker_id);

        Matrix4fRM mvp_matrix = generateEigenModelMatrix(rotation, translation);

        // The generated model matrix must be mapped to TimedModelMatrixProto
        // (col-wise)
        for (int i = 0; i < mvp_matrix.rows(); ++i) {
          for (int j = 0; j < mvp_matrix.cols(); ++j) {
            model_matrix -> add_matrix_entries(mvp_matrix(j, i));
          }
        }
        break;
      }
    }
  }

  cc -> Outputs()
      .Tag(kModelMatricesTag)
      .Add(model_matrices.release(), cc -> InputTimestamp());

  return ::mediapipe::OkStatus();
}

// Generates a model matrix via Eigen with appropriate transformations
Matrix4fRM MatricesManagerCalculator::generateEigenModelMatrix(
    Rotation rotation, Translation translation) {
  Matrix4fRM mvp_matrix;

  mvp_matrix << 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
      0.0, 0.0, 1.0;  // trailing 1.0 required by OpenGL

  // Generate and set the translation vector
  Eigen::Vector3f t_vector(translation.x, translation.y, translation.z);
  mvp_matrix.bottomLeftCorner<1, 3>() = t_vector;

  // Generate and set the rotation submatrix
  Eigen::Matrix3f r_submatrix;
  r_submatrix =
      Eigen::AngleAxisf(rotation.y_radians, Eigen::Vector3f::UnitY()) *
      Eigen::AngleAxisf(rotation.z_radians, Eigen::Vector3f::UnitZ()) *
      Eigen::AngleAxisf(rotation.x_radians, Eigen::Vector3f::UnitX());
  mvp_matrix.topLeftCorner<3, 3>() = r_submatrix;

  return mvp_matrix;
}
}
