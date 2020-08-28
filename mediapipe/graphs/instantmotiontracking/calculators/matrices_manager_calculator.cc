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
  using Matrix4fCM = Eigen::Matrix<float, 4, 4, Eigen::ColMajor>;
  using Vector3f = Eigen::Vector3f;
  using Matrix3f = Eigen::Matrix3f;
  using DiagonalMatrix3f = Eigen::DiagonalMatrix<float, 3>;
  constexpr char kMatricesTag[] = "MATRICES";
  constexpr char kUserRotationsTag[] = "USER_ROTATIONS";
  constexpr char kUserScalingsTag[] = "USER_SCALINGS";
  constexpr char kRendersTag[] = "RENDER_DATA";
  constexpr char kGifAspectRatioTag[] = "GIF_ASPECT_RATIO";
  constexpr char kModelMatricesTag[] = "MODEL_MATRICES";
  // True is GIF aspect ratio input exists
  bool gif_aspect_ratio_exists = false;
}

// Intermediary for rotation and translation data to model matrix usable by
// gl_animation_overlay_calculator. For information on the construction of OpenGL
// objects and transformations (including a breakdown of model matrices), please
// visit: https://open.gl/transformations
//
//
// Input:
//  ANCHORS - Anchor data with x,y,z coordinates (x,y are in [0.0-1.0] range for
//    position on the device screen, while z is the scaling factor that changes
//    in proportion to the distance from the tracked region) [REQUIRED]
//  USER_ROTATIONS - UserRotations with corresponding radians of rotation [REQUIRED]
//  USER_SCALINGS - UserScalings with corresponding scale factor [REQUIRED]
//  GIF_ASPECT_RATIO - Aspect ratio of GIF image used to dynamically scale GIF asset
//  defined as width / height [OPTIONAL]
// Output:
//  MATRICES - TimedModelMatrixProtoList of each object type to render [REQUIRED]
//
// Example config:
// node{
//  calculator: "MatricesManagerCalculator"
//  input_stream: "ANCHORS:tracked_scaled_anchor_data"
//  input_stream: "USER_ROTATIONS:user_rotation_data"
//  input_stream: "USER_SCALINGS:user_scaling_data"
//  input_stream: "GIF_ASPECT_RATIO:gif_aspect_ratio"
//  output_stream: "MATRICES:0:first_render_matrices"
//  output_stream: "MATRICES:1:second_render_matrices" [unbounded input size]
// }

class MatricesManagerCalculator : public CalculatorBase {
  public:
    static ::mediapipe::Status GetContract(CalculatorContract* cc);
    ::mediapipe::Status Open(CalculatorContext* cc) override;
    ::mediapipe::Status Process(CalculatorContext* cc) override;
  private:
    const DiagonalMatrix3f GenerateScalingMatrix(const float scale_factor);
    const Matrix3f GenerateRotationTransformationMatrix(const float rotation_radians);

    // This returns a scale factor by which to alter the projection matrix for
    // the specified render id in order to ensure all objects render at a similar
    // size in the view screen upon initial placement
    const DiagonalMatrix3f GetDefaultRenderScaleDiagonal(const int render_id, const float user_scale_factor, const float gif_aspect_ratio) {
      float scale_preset = 1.0f;
      float x_scalar = 1.0f;
      float y_scalar = 1.0f;

      if (render_id == 0) { // GIF
        // 160 is the scaling preset to make the GIF asset appear relatively
        // similar in size to all other assets
        scale_preset = 160.0f;
        if (gif_aspect_ratio >= 1.0f) {
          // GIF is wider horizontally (scale on x-axis)
          x_scalar = gif_aspect_ratio;
        }
        else {
          // GIF is wider vertically (scale on y-axis)
          y_scalar = 1.0f / gif_aspect_ratio;
        }
      }
      else if (render_id == 1) { // Asset 1
        // 5 is the scaling preset to make the 3D asset appear relatively
        // similar in size to all other assets
        scale_preset = 5.0f;
      }
      DiagonalMatrix3f scaling(scale_preset * user_scale_factor * x_scalar,
        scale_preset * user_scale_factor * y_scalar,
        scale_preset * user_scale_factor);
      return scaling;
    }
};

REGISTER_CALCULATOR(MatricesManagerCalculator);

::mediapipe::Status MatricesManagerCalculator::GetContract(
    CalculatorContract* cc) {
  RET_CHECK(cc->Inputs().HasTag(kMatricesTag)
    && cc->Inputs().HasTag(kUserRotationsTag)
    && cc->Inputs().HasTag(kUserScalingsTag));

  cc->Inputs().Tag(kMatricesTag).Set<std::vector<Matrix4fCM>>();
  cc->Inputs().Tag(kUserScalingsTag).Set<std::vector<float>>();
  cc->Inputs().Tag(kUserRotationsTag).Set<std::vector<float>>();
  cc->Inputs().Tag(kRendersTag).Set<std::vector<int>>();

  // Check for optional Gif aspect ratio input stream
  gif_aspect_ratio_exists = cc->Inputs().HasTag(kGifAspectRatioTag);
  if (gif_aspect_ratio_exists) {
    cc->Inputs().Tag(kGifAspectRatioTag).Set<float>();
  }

  for (CollectionItemId id = cc->Outputs().BeginId("MATRICES");
         id < cc->Outputs().EndId("MATRICES"); ++id) {
           cc->Outputs().Get(id).Set<TimedModelMatrixProtoList>();
  }

  return ::mediapipe::OkStatus();
}

::mediapipe::Status MatricesManagerCalculator::Open(CalculatorContext* cc) {
  cc->SetOffset(TimestampDiff(0));
  return ::mediapipe::OkStatus();
}

::mediapipe::Status MatricesManagerCalculator::Process(CalculatorContext* cc) {
  // Define each object's model matrices
  auto asset_matrices_gif = std::make_unique<TimedModelMatrixProtoList>();
  auto asset_matrices_1 = std::make_unique<TimedModelMatrixProtoList>();
  // Clear all model matrices
  asset_matrices_gif.get()->clear_model_matrix();
  asset_matrices_1.get()->clear_model_matrix();

  const std::vector<float> user_rotation_data =
      cc->Inputs().Tag(kUserRotationsTag).Get<std::vector<float>>();

  const std::vector<float> user_scaling_data =
        cc->Inputs().Tag(kUserScalingsTag).Get<std::vector<float>>();

  const std::vector<int> render_data =
      cc->Inputs().Tag(kRendersTag).Get<std::vector<int>>();

  std::vector<Matrix4fCM> matrix_input_data =
      cc->Inputs()
          .Tag(kMatricesTag)
          .Get<std::vector<Matrix4fCM>>();

  // If no ratio provided, Gif ratio will default to standard GIF.obj size
  const float gif_aspect_ratio = (gif_aspect_ratio_exists) ?
    cc->Inputs().Tag(kGifAspectRatioTag).Get<float>() : 1.0f;

  int idx = 0;
  for (Matrix4fCM &input_matrix : matrix_input_data) {
    TimedModelMatrixProto* model_matrix;
    // Add model matrix to matrices list for defined object render ID
    if (render_data[idx] == 0) { // GIF
      model_matrix = asset_matrices_gif.get()->add_model_matrix();
    }
    else if (render_data[idx] == 1) { // Asset 1
      model_matrix = asset_matrices_1.get()->add_model_matrix();
    }
    model_matrix->set_id(idx);

    const Matrix3f user_rotation_submatrix = GenerateRotationTransformationMatrix(user_rotation_data[idx]);
    DiagonalMatrix3f scaling_diagonal = GetDefaultRenderScaleDiagonal(render_data[idx], user_scaling_data[idx], gif_aspect_ratio);
    // The user transformation data can be concatenated into a final rotation submatrix
    const Matrix3f user_transformation_submatrix = user_rotation_submatrix * scaling_diagonal;

    // Concatenate all model matrix data
    input_matrix.topLeftCorner<3, 3>() = input_matrix.topLeftCorner<3, 3>() * user_transformation_submatrix;

    // Increment to next id from vector
    idx++;

    // The generated model matrix must be mapped to TimedModelMatrixProto (col-wise)
    for (int x = 0; x < input_matrix.rows(); ++x) {
      for (int y = 0; y < input_matrix.cols(); ++y) {
        model_matrix->add_matrix_entries(input_matrix(x, y));
      }
    }
  }

  // Output all individual render matrices
  // TODO: Perform depth ordering with gl_animation_overlay_calculator to render
  // objects in order by depth to allow occlusion.
  cc->Outputs()
          .Get(cc->Outputs().GetId("MATRICES", 0))
          .Add(asset_matrices_gif.release(), cc->InputTimestamp());
  cc->Outputs()
          .Get(cc->Outputs().GetId("MATRICES", 1))
          .Add(asset_matrices_1.release(), cc->InputTimestamp());

  return ::mediapipe::OkStatus();
}

// Using a specified rotation value in radians, generate a rotation matrix for use with base rotation submatrix
const Matrix3f MatricesManagerCalculator::GenerateRotationTransformationMatrix(const float rotation_radians) {
  Eigen::Matrix3f user_rotation_submatrix;
    user_rotation_submatrix =
        // The rotation in radians must be inverted to rotate the object
        // with the direction of finger movement from the user (system dependent)
        Eigen::AngleAxisf(-rotation_radians, Eigen::Vector3f::UnitY()) *
        Eigen::AngleAxisf(0.0f, Eigen::Vector3f::UnitZ()) *
        // Model orientations all assume z-axis is up, but we need y-axis upwards,
        // therefore, a +(M_PI * 0.5f) transformation must be applied
        // TODO: Bring default rotations, translations, and scalings into independent
        // sticker configuration
        Eigen::AngleAxisf(M_PI * 0.5f, Eigen::Vector3f::UnitX());
  // Matrix must be transposed due to the method of submatrix generation in Eigen
  return user_rotation_submatrix.transpose();
}

// Using a specified scale factor, generate a scaling matrix for use with base rotation submatrix
const DiagonalMatrix3f MatricesManagerCalculator::GenerateScalingMatrix(const float scale_factor) {
  DiagonalMatrix3f scaling_matrix(scale_factor, scale_factor, scale_factor);
  return scaling_matrix;
}

}
