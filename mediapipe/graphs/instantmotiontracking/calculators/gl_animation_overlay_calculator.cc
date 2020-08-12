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

#if defined(__ANDROID__)
#include "mediapipe/util/android/asset_manager_util.h"
#else
#include <fstream>
#include <iostream>
#endif

#include <math.h>
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/port/ret_check.h"
#include "mediapipe/framework/port/status.h"
#include "mediapipe/gpu/gl_calculator_helper.h"
#include "mediapipe/gpu/shader_util.h"
#include "mediapipe/graphs/object_detection_3d/calculators/camera_parameters.pb.h"
#include "mediapipe/graphs/object_detection_3d/calculators/gl_animation_overlay_calculator.pb.h"
#include "mediapipe/graphs/object_detection_3d/calculators/model_matrix.pb.h"

namespace mediapipe {

namespace {

#if defined(GL_DEBUG)
#define GLCHECK(command) \
  command;               \
  if (int err = glGetError()) LOG(ERROR) << "GL error detected: " << err;
#else
#define GLCHECK(command) command
#endif

// For ease of use, we prefer ImageFrame on Android and GpuBuffer otherwise.
#if defined(__ANDROID__)
typedef ImageFrame AssetTextureFormat;
#else
typedef GpuBuffer AssetTextureFormat;
#endif

enum { ATTRIB_VERTEX, ATTRIB_TEXTURE_POSITION, ATTRIB_NORMAL, NUM_ATTRIBUTES };
static const int kNumMatrixEntries = 16;
// Loads a texture from an input side packet, and streams in an animation file
// from a filename given in another input side packet, and renders the animation
// over the screen according to the input timestamp and desired animation FPS.
//
// Inputs:
//   VIDEO (GpuBuffer, optional):
//     If provided, the input buffer will be assumed to be unique, and will be
//     consumed by this calculator and rendered to directly.  The output video
//     buffer will then be the released reference to the input video buffer.
//   MODEL_MATRICES (TimedModelMatrixProtoList, optional):
//     If provided, will set the model matrices for the objects to be rendered
//     during future rendering calls.
//   TEXTURE (AssetTextureFormat, optional):
//     Will load a dynamic texture that is overlayed upon the asset if provided
//
// Input side packets:
//   TEXTURE (ImageFrame on Android / GpuBuffer on iOS, optional):
//     Texture to use for the animation asset
//   ASSET (String, optional):
//     Path of animation file (.obj.uuu) that will be used in this calculator
//   CAMERA_PARAMETERS_PROTO_STRING (String, optional):
//     Serialized proto std::string of CameraParametersProto. This is an artifact
//     of the original gl_animation_overlay_calculator, and unused in this demo
// Options:
//   aspect_ratio: the ratio between the rendered image width and height.
//     It will be ignored if CAMERA_PARAMETERS_PROTO_STRING input side packet
//     is provided.
//   vertical_fov_degrees: vertical field of view in degrees.
//     It will be ignored if CAMERA_PARAMETERS_PROTO_STRING input side packet
//     is provided.
//   z_clipping_plane_near: near plane value for z-clipping.
//   z_clipping_plane_far: far plane value for z-clipping.
//   animation_speed_fps: speed at which to cycle through animation frames (in
//     frames per second).
//
// Outputs:
//   OUTPUT, or index 0 (GpuBuffer):
//     Frames filled with the given texture.

// Simple helper-struct for containing the parsed geometry data from a 3D
// animation frame for rendering.

struct TriangleMesh {
  int index_count = 0;  // Needed for glDrawElements rendering call
  std::unique_ptr<float[]> normals = nullptr;
  std::unique_ptr<float[]> vertices = nullptr;
  std::unique_ptr<float[]> texture_coords = nullptr;
  std::unique_ptr<int16[]> triangle_indices = nullptr;
};

typedef std::unique_ptr<float[]> ModelMatrix;

}  // namespace

class GlAnimationOverlayCalculator : public CalculatorBase {
 public:
  GlAnimationOverlayCalculator() {}
  ~GlAnimationOverlayCalculator();

  static ::mediapipe::Status GetContract(CalculatorContract *cc);

  ::mediapipe::Status Open(CalculatorContext *cc) override;
  ::mediapipe::Status Process(CalculatorContext *cc) override;

 private:
  bool has_video_stream_ = false;
  bool has_model_matrix_stream_ = false;

  GlCalculatorHelper helper_;
  bool initialized_ = false;
  GlTexture texture_;
  std::vector<TriangleMesh> triangle_meshes_;

  GLuint renderbuffer_ = 0;
  bool depth_buffer_created_ = false;

  GLuint program_ = 0;
  GLint texture_uniform_ = -1;
  GLint perspective_matrix_uniform_ = -1;
  GLint model_matrix_uniform_ = -1;

  Timestamp animation_start_time_;
  int frame_count_ = 0;
  float animation_speed_fps_;

  std::vector<ModelMatrix> current_model_matrices_;

  // Perspective matrix for rendering, to be applied to all model matrices
  // prior to passing through to the shader as a MVP matrix.  Initialized during
  // first image packet read.
  float perspective_matrix_[kNumMatrixEntries];

  void ComputeAspectRatioAndFovFromCameraParameters(
      const CameraParametersProto &camera_parameters, float *aspect_ratio,
      float *vertical_fov_degrees);
  int GetAnimationFrameIndex(Timestamp timestamp);
  ::mediapipe::Status GlSetup();
  ::mediapipe::Status GlBind(const TriangleMesh &triangle_mesh,
                             const GlTexture &texture);
  ::mediapipe::Status GlRender(const TriangleMesh &triangle_mesh,
                               const float *model_matrix);
  void InitializePerspectiveMatrix(float aspect_ratio,
                                   float vertical_fov_degrees, float z_near,
                                   float z_far);
  void LoadModelMatrices(const TimedModelMatrixProtoList &model_matrices,
                         std::vector<ModelMatrix> *current_model_matrices);

#if !defined(__ANDROID__)
  // Asset loading routine for all non-Android platforms.
  bool LoadAnimation(const std::string &filename);
#else
  // Asset loading for all Android platforms.
  bool LoadAnimationAndroid(const std::string &filename,
                            std::vector<TriangleMesh> *mesh);
  bool ReadBytesFromAsset(AAsset *asset, void *buffer, int num_bytes_to_read);
#endif
};
REGISTER_CALCULATOR(GlAnimationOverlayCalculator);

// static
::mediapipe::Status GlAnimationOverlayCalculator::GetContract(
    CalculatorContract *cc) {
  MP_RETURN_IF_ERROR(
      GlCalculatorHelper::SetupInputSidePackets(&(cc->InputSidePackets())));
  if (cc->Inputs().HasTag("VIDEO")) {
    // Currently used only for size and timestamp.
    cc->Inputs().Tag("VIDEO").Set<GpuBuffer>();
  }
  TagOrIndex(&(cc->Outputs()), "OUTPUT", 0).Set<GpuBuffer>();

  if (cc->Inputs().HasTag("MODEL_MATRICES")) {
    cc->Inputs().Tag("MODEL_MATRICES").Set<TimedModelMatrixProtoList>();
  }

  if (cc->Inputs().HasTag("TEXTURE")) {
    cc->Inputs().Tag("TEXTURE").Set<AssetTextureFormat>();
  }

  if (cc->InputSidePackets().HasTag("TEXTURE")) {
    cc->InputSidePackets().Tag("TEXTURE").Set<AssetTextureFormat>();
  }

  if (cc->InputSidePackets().HasTag("ASSET")) {
    cc->InputSidePackets().Tag("ASSET").Set<std::string>();
  }

  if (cc->InputSidePackets().HasTag("CAMERA_PARAMETERS_PROTO_STRING")) {
    cc->InputSidePackets()
        .Tag("CAMERA_PARAMETERS_PROTO_STRING")
        .Set<std::string>();
  }

  return ::mediapipe::OkStatus();
}

// Helper function for initializing our perspective matrix.
void GlAnimationOverlayCalculator::InitializePerspectiveMatrix(
    float aspect_ratio, float fov_degrees, float z_near, float z_far) {
  // Standard perspective projection matrix calculations.
  const float f = 1.0f / std::tan(fov_degrees * M_PI / 360.0f);
  for (int i = 0; i < kNumMatrixEntries; i++) {
    perspective_matrix_[i] = 0;
  }
  const float denom = 1.0f / (z_near - z_far);
  perspective_matrix_[0] = f / aspect_ratio;
  perspective_matrix_[5] = f;
  perspective_matrix_[10] = (z_near + z_far) * denom;
  perspective_matrix_[11] = -1.0f;
  perspective_matrix_[14] = 2.0f * z_far * z_near * denom;
}

#if defined(__ANDROID__)
// Helper function for reading in a specified number of bytes from an Android
// asset.  Returns true if successfully reads in all bytes into buffer.
bool GlAnimationOverlayCalculator::ReadBytesFromAsset(AAsset *asset,
                                                      void *buffer,
                                                      int num_bytes_to_read) {
  // Most file systems use block sizes of 4KB or 8KB; ideally we'd choose a
  // small multiple of the block size for best input streaming performance, so
  // we go for a reasobably safe buffer size of 8KB = 8*1024 bytes.
  static const int kMaxChunkSize = 8192;

  int bytes_left = num_bytes_to_read;
  int bytes_read = 1;  // any value > 0 here just to start looping.

  // Treat as uint8_t array so we can deal in single byte arithmetic easily.
  uint8_t *currBufferIndex = reinterpret_cast<uint8_t *>(buffer);
  while (bytes_read > 0 && bytes_left > 0) {
    bytes_read = AAsset_read(asset, (void *)currBufferIndex,
                             std::min(bytes_left, kMaxChunkSize));
    bytes_left -= bytes_read;
    currBufferIndex += bytes_read;
  }
  // At least log any I/O errors encountered.
  if (bytes_read < 0) {
    LOG(ERROR) << "Error reading from AAsset: " << bytes_read;
    return false;
  }
  if (bytes_left > 0) {
    // Reached EOF before reading in specified number of bytes.
    LOG(WARNING) << "Reached EOF before reading in specified number of bytes.";
    return false;
  }
  return true;
}

// The below asset streaming code is Android-only, making use of the platform
// JNI helper classes AAssetManager and AAsset.
bool GlAnimationOverlayCalculator::LoadAnimationAndroid(
    const std::string &filename, std::vector<TriangleMesh> *meshes) {
  mediapipe::AssetManager *mediapipe_asset_manager =
      Singleton<mediapipe::AssetManager>::get();
  AAssetManager *asset_manager = mediapipe_asset_manager->GetAssetManager();
  if (!asset_manager) {
    LOG(ERROR) << "Failed to access Android asset manager.";
    return false;
  }

  // New read-bytes stuff here!  First we open file for streaming.
  AAsset *asset = AAssetManager_open(asset_manager, filename.c_str(),
                                     AASSET_MODE_STREAMING);
  if (!asset) {
    LOG(ERROR) << "Failed to open animation asset: " << filename;
    return false;
  }

  // And now, while we are able to stream in more frames, we do so.
  frame_count_ = 0;
  int32 lengths[3];
  while (ReadBytesFromAsset(asset, (void *)lengths, sizeof(lengths[0]) * 3)) {
    // About to start reading the next animation frame.  Stream it in here.
    // Each frame stores first the object counts of its three arrays
    // (vertices, texture coordinates, triangle indices; respectively), and
    // then stores each of those arrays as a byte dump, in order.
    meshes->emplace_back();
    TriangleMesh &triangle_mesh = meshes->back();
    // Try to read in vertices (4-byte floats)
    triangle_mesh.vertices.reset(new float[lengths[0]]);
    if (!ReadBytesFromAsset(asset, (void *)triangle_mesh.vertices.get(),
                            sizeof(float) * lengths[0])) {
      LOG(ERROR) << "Failed to read vertices for frame " << frame_count_;
      return false;
    }

    // Try to read in texture coordinates (4-byte floats)
    triangle_mesh.texture_coords.reset(new float[lengths[1]]);
    if (!ReadBytesFromAsset(asset, (void *)triangle_mesh.texture_coords.get(),
                            sizeof(float) * lengths[1])) {
      LOG(ERROR) << "Failed to read tex-coords for frame " << frame_count_;
      return false;
    }
    // Try to read in indices (2-byte shorts)
    triangle_mesh.index_count = lengths[2];
    triangle_mesh.triangle_indices.reset(new int16[lengths[2]]);
    if (!ReadBytesFromAsset(asset, (void *)triangle_mesh.triangle_indices.get(),
                            sizeof(int16) * lengths[2])) {
      LOG(ERROR) << "Failed to read indices for frame " << frame_count_;
      return false;
    }

    // Set triangle_mesh normals for shader usage
    triangle_mesh.normals.reset(new float[lengths[0]]);

    // Used for storing the vertex normals prior to averaging
    float vertex_normals_sum[lengths[0]];
    float vertex_avg_denom[lengths[0]];

    // Compute every triangle surface normal and store them for later use.
    for (int idx = 0; idx < lengths[2]; idx += 3) {
      int v1 = triangle_mesh.triangle_indices.get()[idx];
      int v2 = triangle_mesh.triangle_indices.get()[idx + 1];
      int v3 = triangle_mesh.triangle_indices.get()[idx + 2];
      // (V1) vertex X,Y,Z indices in triangle_mesh.vertices
      float V1x = triangle_mesh.vertices[v1 * 3];
      float V1y = triangle_mesh.vertices[v1 * 3 + 1];
      float V1z = triangle_mesh.vertices[v1 * 3 + 2];
      // (V2) vertex X,Y,Z indices in triangle_mesh.vertices
      float V2x = triangle_mesh.vertices[v2 * 3];
      float V2y = triangle_mesh.vertices[v2 * 3 + 1];
      float V2z = triangle_mesh.vertices[v2 * 3 + 2];
      // (V3) vertex X,Y,Z indices in triangle_mesh.vertices
      float V3x = triangle_mesh.vertices[v3 * 3];
      float V3y = triangle_mesh.vertices[v3 * 3 + 1];
      float V3z = triangle_mesh.vertices[v3 * 3 + 2];
      // Calculate normals from vertices
      // V2 - V1
      float Ax = V2x - V1x;
      float Ay = V2y - V1y;
      float Az = V2z - V1z;
      // V3 - V1
      float Bx = V3x - V1x;
      float By = V3y - V1y;
      float Bz = V3z - V1z;
      // Calculate cross product
      float normal_x = Ay * Bz - Az * By;
      float normal_y = Az * Bx - Ax * Bz;
      float normal_z = Ax * By - Ay * Bx;
      // Normalize the triangle surface normal
      float product = 0.0f;
      product = product + normal_x * normal_x;
      product = product + normal_y * normal_y;
      product = product + normal_z * normal_z;
      float magnitude = sqrt(product);
      normal_x /= magnitude;
      normal_y /= magnitude;
      normal_z /= magnitude;

      // Add connected normal to each associated vertex
      // It is also necessary to increment each vertex denominator for averaging
      vertex_normals_sum[v1 * 3] += normal_x;
      vertex_normals_sum[v1 * 3 + 1] += normal_y;
      vertex_normals_sum[v1 * 3 + 2] += normal_z;
      vertex_avg_denom[v1 * 3] += 1;
      vertex_avg_denom[v1 * 3 + 1] += 1;
      vertex_avg_denom[v1 * 3 + 2] += 1;
      vertex_normals_sum[v2 * 3] += normal_x;
      vertex_normals_sum[v2 * 3 + 1] += normal_y;
      vertex_normals_sum[v2 * 3 + 2] += normal_z;
      vertex_avg_denom[v2 * 3] += 1;
      vertex_avg_denom[v2 * 3 + 1] += 1;
      vertex_avg_denom[v2 * 3 + 2] += 1;
      vertex_normals_sum[v3 * 3] += normal_x;
      vertex_normals_sum[v3 * 3 + 1] += normal_y;
      vertex_normals_sum[v3 * 3 + 2] += normal_z;
      vertex_avg_denom[v3 * 3] += 1;
      vertex_avg_denom[v3 * 3 + 1] += 1;
      vertex_avg_denom[v3 * 3 + 2] += 1;
    }

    // Combine all triangle normals connected to each vertex by adding the X,Y,Z
    // value of each adjacent triangle surface normal to every vertex and then
    // averaging the combined value.
    for (int idx = 0; idx < lengths[0]; idx+=3) {
      triangle_mesh.normals.get()[idx] = vertex_normals_sum[idx] / vertex_avg_denom[idx];
      triangle_mesh.normals.get()[idx + 1] = vertex_normals_sum[idx + 1] / vertex_avg_denom[idx + 1];
      triangle_mesh.normals.get()[idx + 2] = vertex_normals_sum[idx + 2] / vertex_avg_denom[idx + 2];
    }

    frame_count_++;
  }
  AAsset_close(asset);

  LOG(INFO) << "Finished parsing " << frame_count_ << " animation frames.";
  if (meshes->empty()) {
    LOG(ERROR) << "No animation frames were parsed!  Erroring out calculator.";
    return false;
  }
  return true;
}

#else  // defined(__ANDROID__)

bool GlAnimationOverlayCalculator::LoadAnimation(const std::string &filename) {
  std::ifstream infile(filename.c_str(), std::ifstream::binary);
  if (!infile) {
    LOG(ERROR) << "Error opening asset with filename: " << filename;
    return false;
  }

  frame_count_ = 0;
  int32 lengths[3];
  while (true) {
    // See if we have more initial size counts to read in.
    infile.read((char *)(lengths), sizeof(lengths[0]) * 3);
    if (!infile) {
      // No more frames to read.  Close out.
      infile.close();
      break;
    }

    robot_triangle_meshes_.emplace_back();
    TriangleMesh &triangle_mesh = robot_triangle_meshes_.back();

    // Try to read in vertices (4-byte floats).
    triangle_mesh.vertices.reset(new float[lengths[0]]);
    infile.read((char *)(triangle_mesh.vertices.get()),
                sizeof(float) * lengths[0]);
    if (!infile) {
      LOG(ERROR) << "Failed to read vertices for frame " << frame_count_;
      return false;
    }

    // Try to read in texture coordinates (4-byte floats)
    triangle_mesh.texture_coords.reset(new float[lengths[1]]);
    infile.read((char *)(triangle_mesh.texture_coords.get()),
                sizeof(float) * lengths[1]);
    if (!infile) {
      LOG(ERROR) << "Failed to read texture coordinates for frame "
                 << frame_count_;
      return false;
    }

    // Try to read in the triangle indices (2-byte shorts)
    triangle_mesh.index_count = lengths[2];
    triangle_mesh.triangle_indices.reset(new int16[lengths[2]]);
    infile.read((char *)(triangle_mesh.triangle_indices.get()),
                sizeof(int16) * lengths[2]);
    if (!infile) {
      LOG(ERROR) << "Failed to read triangle indices for frame "
                 << frame_count_;
      return false;
    }
    frame_count_++;
  }

  LOG(INFO) << "Finished parsing " << frame_count_ << " animation frames.";
  if (robot_triangle_meshes_.empty()) {
    LOG(ERROR) << "No animation frames were parsed!  Erroring out calculator.";
    return false;
  }
  return true;
}

#endif

void GlAnimationOverlayCalculator::ComputeAspectRatioAndFovFromCameraParameters(
    const CameraParametersProto &camera_parameters, float *aspect_ratio,
    float *vertical_fov_degrees) {
  CHECK(aspect_ratio != nullptr);
  CHECK(vertical_fov_degrees != nullptr);
  *aspect_ratio =
      camera_parameters.portrait_width() / camera_parameters.portrait_height();
  *vertical_fov_degrees =
      std::atan(camera_parameters.portrait_height() * 0.5f) * 2 * 180 / M_PI;
}

::mediapipe::Status GlAnimationOverlayCalculator::Open(CalculatorContext *cc) {
  cc->SetOffset(TimestampDiff(0));
  MP_RETURN_IF_ERROR(helper_.Open(cc));

  const auto &options = cc->Options<GlAnimationOverlayCalculatorOptions>();

  animation_speed_fps_ = options.animation_speed_fps();

  // Construct projection matrix using input side packets or option
  float aspect_ratio;
  float vertical_fov_degrees;
  if (cc->InputSidePackets().HasTag("CAMERA_PARAMETERS_PROTO_STRING")) {
    const std::string &camera_parameters_proto_string =
        cc->InputSidePackets()
            .Tag("CAMERA_PARAMETERS_PROTO_STRING")
            .Get<std::string>();
    CameraParametersProto camera_parameters_proto;
    camera_parameters_proto.ParseFromString(camera_parameters_proto_string);
    ComputeAspectRatioAndFovFromCameraParameters(
        camera_parameters_proto, &aspect_ratio, &vertical_fov_degrees);
  } else {
    aspect_ratio = options.aspect_ratio();
    vertical_fov_degrees = options.vertical_fov_degrees();
  }

  // when constructing projection matrix.
  InitializePerspectiveMatrix(aspect_ratio, vertical_fov_degrees,
                              options.z_clipping_plane_near(),
                              options.z_clipping_plane_far());

  // See what streams we have.
  has_video_stream_ = cc->Inputs().HasTag("VIDEO");
  has_model_matrix_stream_ = cc->Inputs().HasTag("MODEL_MATRICES");

  bool loaded_animation = false;

  if (cc->InputSidePackets().HasTag("ASSET")) {
    const std::string &asset_name = cc->InputSidePackets().Tag("ASSET").Get<std::string>();
    #if defined(__ANDROID__)
      loaded_animation = LoadAnimationAndroid(asset_name, &triangle_meshes_);
    #else
      loaded_animation = LoadAnimation(asset_name);
    #endif
  }

    if (!loaded_animation) {
      LOG(ERROR) << "Failed to load animation asset.";
      return ::mediapipe::UnknownError("Failed to load animation asset.");
    }

  return helper_.RunInGlContext([this, &cc]() -> ::mediapipe::Status {

    // Load in all static texture data if it exists
    if (cc->InputSidePackets().HasTag("TEXTURE")) {
      const auto &input_texture =
          cc->InputSidePackets().Tag("TEXTURE").Get<AssetTextureFormat>();
      texture_ = helper_.CreateSourceTexture(input_texture);
    }

    return ::mediapipe::OkStatus();
  });
}

int GlAnimationOverlayCalculator::GetAnimationFrameIndex(Timestamp timestamp) {
  double seconds_delta = timestamp.Seconds() - animation_start_time_.Seconds();
  int64_t frame_index =
      static_cast<int64_t>(seconds_delta * animation_speed_fps_);
  frame_index %= frame_count_;
  return static_cast<int>(frame_index);
}

void GlAnimationOverlayCalculator::LoadModelMatrices(
    const TimedModelMatrixProtoList &model_matrices,
    std::vector<ModelMatrix> *current_model_matrices) {
  current_model_matrices->clear();
  for (int i = 0; i < model_matrices.model_matrix_size(); ++i) {
    const auto &model_matrix = model_matrices.model_matrix(i);
    CHECK(model_matrix.matrix_entries_size() == kNumMatrixEntries)
        << "Invalid Model Matrix";
    current_model_matrices->emplace_back();
    ModelMatrix &new_matrix = current_model_matrices->back();
    new_matrix.reset(new float[kNumMatrixEntries]);
    for (int j = 0; j < kNumMatrixEntries; j++) {
      // Model matrices streamed in using ROW-MAJOR format, but we want
      // COLUMN-MAJOR for rendering, so we transpose here.
      int col = j % 4;
      int row = j / 4;
      new_matrix[row + col * 4] = model_matrix.matrix_entries(j);
    }
  }
}

::mediapipe::Status GlAnimationOverlayCalculator::Process(
    CalculatorContext *cc) {
  return helper_.RunInGlContext([this, &cc]() -> ::mediapipe::Status {
    if (!initialized_) {
      MP_RETURN_IF_ERROR(GlSetup());
      initialized_ = true;
      animation_start_time_ = cc->InputTimestamp();
    }

    // Process model matrices, if any are being streamed in, and update our
    // list.
    if (has_model_matrix_stream_ &&
        !cc->Inputs().Tag("MODEL_MATRICES").IsEmpty()) {
      const TimedModelMatrixProtoList &model_matrices =
          cc->Inputs().Tag("MODEL_MATRICES").Get<TimedModelMatrixProtoList>();
      LoadModelMatrices(model_matrices, &current_model_matrices_);
    }

    // Arbitrary default width and height for output destination texture, in the
    // event that we don't have a valid and unique input buffer to overlay.
    int width = 640;
    int height = 480;

    GlTexture dst;
    std::unique_ptr<GpuBuffer> input_frame(nullptr);
    if (has_video_stream_ && !(cc->Inputs().Tag("VIDEO").IsEmpty())) {
      auto result = cc->Inputs().Tag("VIDEO").Value().Consume<GpuBuffer>();
      if (result.ok()) {
        input_frame = std::move(result).ValueOrDie();
#if !MEDIAPIPE_GPU_BUFFER_USE_CV_PIXEL_BUFFER
        input_frame->GetGlTextureBufferSharedPtr()->Reuse();
#endif
        width = input_frame->width();
        height = input_frame->height();
        dst = helper_.CreateSourceTexture(*input_frame);
      } else {
        LOG(ERROR) << "Unable to consume input video frame for overlay!";
        LOG(ERROR) << "Status returned was: " << result.status();
        dst = helper_.CreateDestinationTexture(width, height);
      }
    } else if (!has_video_stream_) {
      dst = helper_.CreateDestinationTexture(width, height);
    } else {
      // We have an input video stream, but not for this frame. Don't render!
      return ::mediapipe::OkStatus();
    }
    helper_.BindFramebuffer(dst);

    if (!depth_buffer_created_) {
      // Create our private depth buffer.
      GLCHECK(glGenRenderbuffers(1, &renderbuffer_));
      GLCHECK(glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer_));
      GLCHECK(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                                    width, height));
      GLCHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                        GL_RENDERBUFFER, renderbuffer_));
      GLCHECK(glBindRenderbuffer(GL_RENDERBUFFER, 0));
      depth_buffer_created_ = true;
    }

    // Re-bind our depth renderbuffer to our FBO depth attachment here.
    GLCHECK(glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer_));
    GLCHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                      GL_RENDERBUFFER, renderbuffer_));
    GLenum status = GLCHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER));
    if (status != GL_FRAMEBUFFER_COMPLETE) {
      LOG(ERROR) << "Incomplete framebuffer with status: " << status;
    }
    GLCHECK(glClear(GL_DEPTH_BUFFER_BIT));

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    int frame_index = GetAnimationFrameIndex(cc->InputTimestamp());

    // Define current frame for animation asset
    TriangleMesh &current_frame = triangle_meshes_[frame_index];

    // Load dynamic texture if it exists
    if (cc->Inputs().HasTag("TEXTURE")) {
      const auto &input_texture =
          cc->Inputs().Tag("TEXTURE").Get<AssetTextureFormat>();
      texture_ = helper_.CreateSourceTexture(input_texture);
    }

    if (has_model_matrix_stream_) {
      // Bind texture being used
      MP_RETURN_IF_ERROR(GlBind(current_frame, texture_));
      for (const ModelMatrix &model_matrix : current_model_matrices_) {
          MP_RETURN_IF_ERROR(GlRender(current_frame, model_matrix.get()));
      }
    }
    else {
      // Do nothing.
    }

    // Disable vertex attributes
    GLCHECK(glDisableVertexAttribArray(ATTRIB_VERTEX));
    GLCHECK(glDisableVertexAttribArray(ATTRIB_TEXTURE_POSITION));
    GLCHECK(glDisableVertexAttribArray(ATTRIB_NORMAL));

    // Disable depth test
    GLCHECK(glDisable(GL_DEPTH_TEST));

    // Unbind texture
    GLCHECK(glActiveTexture(GL_TEXTURE1));
    GLCHECK(glBindTexture(texture_.target(), 0));

    // Unbind depth buffer
    GLCHECK(glBindRenderbuffer(GL_RENDERBUFFER, 0));

    GLCHECK(glFlush());

    auto output = dst.GetFrame<GpuBuffer>();
    dst.Release();
    TagOrIndex(&(cc->Outputs()), "OUTPUT", 0)
        .Add(output.release(), cc->InputTimestamp());
    GLCHECK(glFrontFace(GL_CCW));
    return ::mediapipe::OkStatus();
  });
}

::mediapipe::Status GlAnimationOverlayCalculator::GlSetup() {
  // Load vertex and fragment shaders
  const GLint attr_location[NUM_ATTRIBUTES] = {
      ATTRIB_VERTEX,
      ATTRIB_TEXTURE_POSITION,
      ATTRIB_NORMAL,
  };
  const GLchar *attr_name[NUM_ATTRIBUTES] = {
      "position",
      "texture_coordinate",
      "normal",
  };

  const GLchar *vert_src = R"(
    // Perspective projection matrix for rendering / clipping
    uniform mat4 perspectiveMatrix;

    // Matrix defining the currently rendered object model
    uniform mat4 modelMatrix;

    // vertex position in threespace
    attribute vec4 position;
    attribute vec3 normal;

    // texture coordinate for each vertex in normalized texture space (0..1)
    attribute mediump vec4 texture_coordinate;

    // texture coordinate for fragment shader (will be interpolated)
    varying mediump vec2 sampleCoordinate;
    varying mediump vec3 vNormal;

    void main() {
      sampleCoordinate = texture_coordinate.xy;
      mat4 mvpMatrix = perspectiveMatrix * modelMatrix;
      gl_Position = mvpMatrix * position;

      // TODO: Pass in rotation submatrix with no scaling or transforms to prevent
      // breaking vNormal in case of model matrix having non-uniform scaling
      vec4 tmpNormal = mvpMatrix * vec4(normal, 1.0);
      vec4 transformedZero = mvpMatrix * vec4(0.0, 0.0, 0.0, 1.0);
      tmpNormal = tmpNormal - transformedZero;
      vNormal = normalize(tmpNormal.xyz);
    }
  )";

  const GLchar *frag_src = R"(
    precision mediump float;

    varying vec2 sampleCoordinate;  // texture coordinate (0..1)
    varying vec3 vNormal;
    uniform sampler2D texture;  // texture to shade with
    const float kPi = 3.14159265359;

    // Define ambient lighting factor that is applied to our texture in order to
    // generate ambient lighting of the scene on the object. Range is [0.0-1.0],
    // with the factor being proportional to the brightness of the lighting in the
    // scene being applied to the object
    const float kAmbientLighting = 0.9;

    // Define RGB values for light source
    const vec3 kLightColor = vec3(1.0);
    // Exponent for directional lighting that governs diffusion of surface light
    const float kExponent = 1.0;
    // Define direction of lighting effect source
    const vec3 lightDir = vec3(0.0, -1.0, -0.6);
    // Hard-coded view direction
    const vec3 viewDir = vec3(0.0, 0.0, -1.0);

    // DirectionalLighting procedure imported from Lullaby @ https://github.com/google/lullaby
    // Calculate and return the color (diffuse and specular together) reflected by
    // a directional light.
    vec3 GetDirectionalLight(vec3 pos, vec3 normal, vec3 viewDir, vec3 lightDir, vec3 lightColor, float exponent) {
      // Intensity of the diffuse light. Saturate to keep within the 0-1 range.
      float normal_dot_light_dir = dot(normal, -lightDir);
      float intensity = clamp(normal_dot_light_dir, 0.0, 1.0);
      // Calculate the diffuse light
      vec3 diffuse = intensity * lightColor;
      // http://www.rorydriscoll.com/2009/01/25/energy-conservation-in-games/
      float kEnergyConservation = (2.0 + exponent) / (2.0 * kPi);
      vec3 reflect_dir = reflect(lightDir, normal);
      // Intensity of the specular light
      float view_dot_reflect = dot(-viewDir, reflect_dir);
      // Use an epsilon for pow because pow(x,y) is undefined if x < 0 or x == 0
      // and y <= 0 (GLSL Spec 8.2)
      const float kEpsilon = 1e-5;
      intensity = kEnergyConservation * pow(clamp(view_dot_reflect, kEpsilon, 1.0),
       exponent);
      // Specular color:
      vec3 specular = intensity * lightColor;
      return diffuse + specular;
    }

    void main() {
      // Sample the texture, retrieving an rgba pixel value
      vec4 pixel = texture2D(texture, sampleCoordinate);
      // If the alpha (background) value is near transparent, then discard the
      // pixel, this allows the rendering of transparent background GIFs
      if (pixel.a < 0.2) discard;

      // Convert pixel to 3D for lighting procedure
      vec3 pixelPos = vec3(sampleCoordinate, 1.0);
      // Generate directional lighting effect
      vec3 lighting = GetDirectionalLight(pixelPos, vNormal, viewDir, lightDir, kLightColor, kExponent);
      // Apply both ambient and directional lighting to our texture
      gl_FragColor = vec4((vec3(kAmbientLighting) + lighting) * pixel.rgb, 1.0);
    }
  )";

  // Shader program
  GLCHECK(GlhCreateProgram(vert_src, frag_src, NUM_ATTRIBUTES,
                           (const GLchar **)&attr_name[0], attr_location,
                           &program_));
  RET_CHECK(program_) << "Problem initializing the program.";
  texture_uniform_ = GLCHECK(glGetUniformLocation(program_, "texture"));
  perspective_matrix_uniform_ =
      GLCHECK(glGetUniformLocation(program_, "perspectiveMatrix"));
  model_matrix_uniform_ =
      GLCHECK(glGetUniformLocation(program_, "modelMatrix"));
  return ::mediapipe::OkStatus();
}

::mediapipe::Status GlAnimationOverlayCalculator::GlBind(
    const TriangleMesh &triangle_mesh, const GlTexture &texture) {
  GLCHECK(glUseProgram(program_));

  // Disable backface culling to allow occlusion effects.
  // Some options for solid arbitrary 3D geometry rendering
  GLCHECK(glEnable(GL_BLEND));
  GLCHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
  GLCHECK(glEnable(GL_DEPTH_TEST));
  GLCHECK(glFrontFace(GL_CW));
  GLCHECK(glDepthMask(GL_TRUE));
  GLCHECK(glDepthFunc(GL_LESS));

  // Clear our depth buffer before starting draw calls
  GLCHECK(glVertexAttribPointer(ATTRIB_VERTEX, 3, GL_FLOAT, 0, 0,
                                triangle_mesh.vertices.get()));
  GLCHECK(glEnableVertexAttribArray(ATTRIB_VERTEX));

  GLCHECK(glVertexAttribPointer(ATTRIB_TEXTURE_POSITION, 2, GL_FLOAT, 0, 0,
                                triangle_mesh.texture_coords.get()));
  GLCHECK(glEnableVertexAttribArray(ATTRIB_TEXTURE_POSITION));

  GLCHECK(glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, 0, 0,
                                triangle_mesh.normals.get()));
  GLCHECK(glEnableVertexAttribArray(ATTRIB_NORMAL));

  GLCHECK(glActiveTexture(GL_TEXTURE1));
  GLCHECK(glBindTexture(texture.target(), texture.name()));

  // We previously bound it to GL_TEXTURE1
  GLCHECK(glUniform1i(texture_uniform_, 1));

  GLCHECK(glUniformMatrix4fv(perspective_matrix_uniform_, 1, GL_FALSE,
                             perspective_matrix_));
  return ::mediapipe::OkStatus();
}

::mediapipe::Status GlAnimationOverlayCalculator::GlRender(
    const TriangleMesh &triangle_mesh, const float *model_matrix) {
  GLCHECK(glUniformMatrix4fv(model_matrix_uniform_, 1, GL_FALSE, model_matrix));
  GLCHECK(glDrawElements(GL_TRIANGLES, triangle_mesh.index_count,
                         GL_UNSIGNED_SHORT,
                         triangle_mesh.triangle_indices.get()));
  return ::mediapipe::OkStatus();
}

GlAnimationOverlayCalculator::~GlAnimationOverlayCalculator() {
  helper_.RunInGlContext([this] {
    if (program_) {
      GLCHECK(glDeleteProgram(program_));
      program_ = 0;
    }
    if (depth_buffer_created_) {
      GLCHECK(glDeleteRenderbuffers(1, &renderbuffer_));
      renderbuffer_ = 0;
    }
    // Release all textures
    if (texture_.width() > 0) {
      texture_.Release();
    }
  });
}

}  // namespace mediapipe
