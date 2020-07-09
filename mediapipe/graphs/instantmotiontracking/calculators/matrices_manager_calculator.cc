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
  constexpr char kAnchorsTag[] = "ANCHORS";
  constexpr char kIMUDataTag[] = "IMU_DATA";
  constexpr char kUserRotationsTag[] = "USER_ROTATIONS";
  constexpr char kUserScalingsTag[] = "USER_SCALINGS";
  constexpr char kModelMatricesTag[] = "MODEL_MATRICES";
  constexpr char kFOVSidePacketTag[] = "FOV";
  constexpr char kAspectRatioSidePacketTag[] = "ASPECT_RATIO";
  // (68 degrees, 4:3 for Pixel 4)
  float vertical_fov_radians_ = 0;
  float aspect_ratio_ = 0;
  // initial Z value (-98 is just in visual range for OpenGL render)
  const float initial_z_ = -98;
}

// Intermediary for rotation and translation data to model matrix usable by
// gl_animation_overlay_calculator. For information on the construction of OpenGL
// objects and transformations (including a breakdown of model matrices), please
// visit: https://open.gl/transformations
//
// Input Side Packets:
//  FOV - Vertical field of view for device [REQUIRED - Defines perspective matrix]
//  ASPECT_RATIO - Aspect ratio of device [REQUIRED - Defines perspective matrix]
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
    const DiagonalMatrix3f GenerateUserScalingMatrix(const float scale_factor);
    const Matrix3f GenerateUserRotationMatrix(const float rotation_radians);
    const Matrix4fCM GenerateEigenModelMatrix(const Vector3f translation_vector,
      const Matrix3f rotation_submatrix);
    const Vector3f GenerateAnchorVector(const Anchor tracked_anchor);
    const Matrix3f GenerateIMURotationSubmatrix(const float yaw, const float pitch, const float roll);

    // Returns a user scaling increment associated with the sticker_id
    // TODO: Adjust lookup function if total number of stickers is uncapped to improve performance
    const float GetUserScaler(const std::vector<UserScaling> scalings, const int sticker_id) {
      for (const UserScaling &user_scaling : scalings) {
        if (user_scaling.sticker_id == sticker_id) {
          return user_scaling.scaling_increment;
        }
      }
    }
    // Returns a user rotation in radians associated with the sticker_id
    const float GetUserRotation(const std::vector<UserRotation> rotations, const int sticker_id) {
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
    && cc->Inputs().HasTag(kUserScalingsTag)
    && cc->InputSidePackets().HasTag(kFOVSidePacketTag)
    && cc->InputSidePackets().HasTag(kAspectRatioSidePacketTag));
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

  cc->InputSidePackets().Tag(kFOVSidePacketTag).Set<float>();
  cc->InputSidePackets().Tag(kAspectRatioSidePacketTag).Set<float>();

  return ::mediapipe::OkStatus();
}

::mediapipe::Status MatricesManagerCalculator::Open(CalculatorContext* cc) {
  cc->SetOffset(TimestampDiff(0));
  // Set device properties from side packets
  vertical_fov_radians_ = cc->InputSidePackets().Tag(kFOVSidePacketTag).Get<float>();
  aspect_ratio_ = cc->InputSidePackets().Tag(kAspectRatioSidePacketTag).Get<float>();
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
  const float yaw = cc->Inputs().Tag(kIMUDataTag).Get<float[]>()[0];
  const float pitch = cc->Inputs().Tag(kIMUDataTag).Get<float[]>()[1];
  const float roll = cc->Inputs().Tag(kIMUDataTag).Get<float[]>()[2];
  // We can pre-generate the IMU submatrix as this remains unchanged for all objects
  const Matrix3f imu_rotation_submatrix = GenerateIMURotationSubmatrix(yaw, pitch, roll);

  for (const Anchor &anchor : translation_data) {
    const int id = anchor.sticker_id;

    TimedModelMatrixProto* model_matrix =
        model_matrix_list->add_model_matrix();
    model_matrix->set_id(id);

    // The user transformation data associate with this sticker must be defined
    const float rotation = GetUserRotation(user_rotation_data, id);
    const float scaler = GetUserScaler(user_scaling_data, id);

    // A vector representative of a user's sticker rotation transformation can be created
    const Matrix3f user_rotation_submatrix = GenerateUserRotationMatrix(rotation);
    // The user transformation data can be concatenated into a final rotation submatrix with the
    // device IMU rotational data
    const Matrix3f rotation_submatrix = imu_rotation_submatrix * user_rotation_submatrix;

    // Next, the submatrix representative of the user's scaling transformation must be generated
    const DiagonalMatrix3f user_scaling_submatrix = GenerateUserScalingMatrix(scaler);

    // A vector representative of the translation of the object in OpenGL coordinate space must be generated
    const Vector3f translation_vector = GenerateAnchorVector(anchor);

    // Concatenate all model matrix data
    const Matrix4fCM final_model_matrix = GenerateEigenModelMatrix(translation_vector, user_scaling_submatrix * rotation_submatrix);

    // The generated model matrix must be mapped to TimedModelMatrixProto (col-wise)
    for (int x = 0; x < final_model_matrix.rows(); ++x) {
      for (int y = 0; y < final_model_matrix.cols(); ++y) {
        model_matrix->add_matrix_entries(final_model_matrix(x, y));
      }
    }
  }

  cc->Outputs()
      .Tag(kModelMatricesTag)
      .Add(model_matrices.release(), cc->InputTimestamp());

  return ::mediapipe::OkStatus();
}

// Using a specified rotation value in radians, generate a rotation matrix for use with base rotation submatrix
const Matrix3f MatricesManagerCalculator::GenerateUserRotationMatrix(const float rotation_radians) {
  Eigen::Matrix3f user_rotation_submatrix;
    user_rotation_submatrix =
        // The rotation in radians must be inverted to rotate the object
        // with the direction of finger movement from the user (system dependent)
        Eigen::AngleAxisf(-rotation_radians, Eigen::Vector3f::UnitY()) *
        Eigen::AngleAxisf(0, Eigen::Vector3f::UnitZ()) *
        Eigen::AngleAxisf(0, Eigen::Vector3f::UnitX());
  // Matrix must be transposed due to the method of submatrix generation in Eigen
  return user_rotation_submatrix.transpose();
}

// Using a specified scale factor, generate a scaling matrix for use with base rotation submatrix
const DiagonalMatrix3f MatricesManagerCalculator::GenerateUserScalingMatrix(const float scale_factor) {
  DiagonalMatrix3f scaling_matrix(scale_factor, scale_factor, scale_factor);
  return scaling_matrix;
}

// TODO: Investigate possible differences in warping of tracking speed across screen
// Using the sticker anchor data, a translation vector can be generated in OpenGL coordinate space
const Vector3f MatricesManagerCalculator::GenerateAnchorVector(const Anchor tracked_anchor) {
  // Using an initial_z value in OpenGL space, generate a new base z-axis value to mimic scaling by distance.
  const float z = initial_z_ * tracked_anchor.z;

  // Using triangle geometry, the minimum for a y-coordinate that will appear in the view field
  // for the given z value above can be found.
  const float y_minimum = z * (tan(vertical_fov_radians_ / 2));

  // The aspect ratio of the device and y_minimum calculated above can be used to find the
  // minimum value for x that will appear in the view field of the device screen.
  const float x_minimum = y_minimum * (1.0 / aspect_ratio_);

  // Given the minimum bounds of the screen in OpenGL space, the tracked anchor coordinates
  // can be converted to OpenGL coordinate space.
  //
  // (i.e: X and Y will be converted from [0.0-1.0] space to [x_minimum, -x_minimum] space
  // and [y_minimum, -y_minimum] space respectively)
  const float x = (-2 * tracked_anchor.x * x_minimum) + x_minimum;
  const float y = (-2 * tracked_anchor.y * y_minimum) + y_minimum;

  // A translation transformation vector can be generated via Eigen
  const Vector3f t_vector(x,y,z);
  return t_vector;
}

// Using the yaw, pitch, and roll, a rotation submatrix can be generated,
// universal to each object appearing in the device view
const Matrix3f MatricesManagerCalculator::GenerateIMURotationSubmatrix(
  const float yaw, const float pitch, const float roll) {
  Eigen::Matrix3f r_submatrix;
    r_submatrix =
        // The yaw value is associated with the Y-axis.
        Eigen::AngleAxisf(yaw, Eigen::Vector3f::UnitY()) *
        // The roll value is associated with the Z-axis.
        Eigen::AngleAxisf(roll, Eigen::Vector3f::UnitZ()) *
        // The pitch value is associated with the X-axis.
        // The (-M_PI/2) must be added in order to adjust the default
        // rendering of the object (the object should appear in the upright
        // orientation upon initial render of the scene - this is entirely
        // dependent on the construction of the .obj file).
        Eigen::AngleAxisf(pitch - (M_PI / 2), Eigen::Vector3f::UnitX());
    // Matrix must be transposed due to the method of submatrix generation in Eigen
    return r_submatrix.transpose();
}

// Generates a model matrix via Eigen with appropriate transformations
const Matrix4fCM MatricesManagerCalculator::GenerateEigenModelMatrix(
    Vector3f translation_vector, Matrix3f rotation_submatrix) {
  // Define basic empty model matrix
  Matrix4fCM mvp_matrix;

  // Set the translation vector
  mvp_matrix.topRightCorner<3, 1>() = translation_vector;

  // Set the rotation submatrix
  mvp_matrix.topLeftCorner<3, 3>() = rotation_submatrix;

  // Set trailing 1.0 required by OpenGL to define coordinate space
  mvp_matrix(3,3) = 1.0;

  return mvp_matrix;
}
}
