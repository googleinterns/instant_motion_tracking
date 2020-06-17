# Instant Motion Tracking

**This is not an officially supported Google product.**

This repository contains the Instant Motion Tracking project
using MediaPipe.

This document focuses on the [below graph](#main-graph) that performs instant motion tracking.

For overall context on instant motion tracking, please read this [Google Developers Blog - TODO](#).

![instant_motion_tracking_android_gif1](images/demo_1.gif) ![instant_motion_tracking_android_gif2](images/demo_2.gif)

## Android

[Source](mediapipe/examples/android/src/java/com/google/mediapipe/apps/instantmotiontracking)

To build and install the app, the MediaPipe repository must first be cloned from Github:

```bash
git clone https://github.com/google/mediapipe.git
```

Once you have the MediaPipe repository, clone this project as well:

```bash
git clone https://github.com/googleinterns/instant_motion_tracking.git
```

Due to BUILD dependency issues with related MediaPipe projects, you must change the visibility permissions of the object_tracking_3d calculators. Run the command below to copy the modified BUILD file to fix privacy issues between projects:

```base
mv instant_motion_tracking/mediapipe/graphs/instantmotiontracking/calculators/obj3d/BUILD mediapipe/mediapipe/graphs/object_detection_3d/calculators/BUILD
```

Next, run the following command to move the instantmotiontracking source graphs and calculators into the MediaPipe directory:

```bash
mv instant_motion_tracking/mediapipe/graphs/instantmotiontracking mediapipe/mediapipe/graphs/instantmotiontracking
```

It is also necessary move the Android source application to Mediapipe:

```bash
mv instant_motion_tracking/mediapipe/examples/android/src/java/com/google/mediapipe/apps/instantmotiontracking mediapipe/mediapipe/examples/android/src/java/com/google/mediapipe/apps/instantmotiontracking
```

Now, navigate to the MediaPipe directory and run the following command to build the application with Bazel

```bash
bazel build -c opt --config=android_arm64 mediapipe/examples/android/src/java/com/google/mediapipe/apps/instantmotiontracking
```

Once the app is built, install it on an Android device with:

```bash
adb install bazel-bin/mediapipe/examples/android/src/java/com/google/mediapipe/apps/instantmotiontracking/instantmotiontracking.apk
```

## Graph
The instant motion tracking [main graph](#main-graph) internally utilizes a [region tracking subgraph](#region-tracking-subgraph)
in order to perform anchor tracking for each individual 3d render (creating the AR effect).

### Main Graph

[Source pbtxt file](mediapipe/graphs/instantmotiontracking/instantmotiontracking.pbtxt)

![main-graph-png](images/main-graph.png)

```bash
# MediaPipe graph that performs region tracking and 3d object (AR sticker) rendering.

# Images in/out of graph with sticker data and IMU information from device
input_stream: "input_video"
input_stream: "sticker_data_string"
input_stream: "imu_data"
output_stream: "output_video"

# Converts sticker data into user data (rotations/scalings), render data, and
# initial anchors.
node {
  calculator: "StickerManagerCalculator"
  input_stream: "STRING:sticker_data_string"
  output_stream: "ANCHORS:initial_anchor_data"
  output_stream: "USER_ROTATIONS:user_rotation_data"
  output_stream: "USER_SCALINGS:user_scaling_data"
  output_stream: "RENDER_DATA:sticker_render_data"
}

# Uses box tracking in order to create 'anchors' for associated 3d stickers.
node {
  calculator: "RegionTrackingSubgraph"
  input_stream: "VIDEO:input_video"
  input_stream: "ANCHORS:initial_anchor_data"
  output_stream: "ANCHORS:tracked_scaled_anchor_data"
}

# Finalizes all translation data and converts coordinates from normalized device
# system to OpenGL coordinates.
node {
  calculator: "TranslationManagerCalculator"
  input_stream: "ANCHORS:tracked_scaled_anchor_data"
  input_stream: "USER_SCALINGS:user_scaling_data"
  output_stream: "TRANSLATION_DATA:final_translation_data"
}

# Finalizes all rotation data and integrates IMU information into individual
# sticker rotation data.
node {
  calculator: "RotationManagerCalculator"
  input_stream: "IMU_DATA:imu_data"
  input_stream: "USER_ROTATIONS:user_rotation_data"
  output_stream: "ROTATION_DATA:final_rotation_data"
}

# Concatenates all transformations to generate model matrices for the OpenGL
# animation overlay calculator.
node {
  calculator: "MatricesManagerCalculator"
  input_stream: "TRANSLATION_DATA:final_translation_data"
  input_stream: "ROTATION_DATA:final_rotation_data"
  output_stream: "MODEL_MATRICES:model_matrices"
}

# Renders the final 3d stickers and overlays them on input image.
node {
  calculator: "GlAnimationOverlayCalculator"
  input_stream: "VIDEO:input_video"
  input_stream: "MODEL_MATRICES:model_matrices"
  output_stream: "output_video"
  input_side_packet: "TEXTURE:box_texture"
  input_side_packet: "ANIMATION_ASSET:box_asset_name"
  input_side_packet: "MASK_TEXTURE:obj_texture"
  input_side_packet: "MASK_ASSET:obj_asset_name"
}
```

### Region Tracking Subgraph

![region_tracking_subgraph](images/anchor-tracking-subgraph.png)

[Source pbtxt file](mediapipe/graphs/instantmotiontracking/subgraphs/region_tracking.pbtxt)

```bash
# MediaPipe graph that performs region tracking on initial anchor positions
# provided by the application

# Images in/out of graph with tracked and scaled normalized anchor data
type: "RegionTrackingSubgraph"
input_stream: "VIDEO:input_video"
input_stream: "ANCHORS:initial_anchor_data"
output_stream: "ANCHORS:tracked_scaled_anchor_data"

# Manages the anchors and tracking if user changes/adds/deletes anchors
node {
 calculator: "TrackedAnchorManagerCalculator"
 input_stream: "ANCHORS:initial_anchor_data"
 input_stream: "BOXES:boxes"
 input_stream_info: {
   tag_index: 'BOXES'
   back_edge: true
 }
 output_stream: "START_POS:start_pos"
 output_stream: "CANCEL_ID:cancel_object_id"
 output_stream: "ANCHORS:tracked_scaled_anchor_data"
}

# Subgraph performs anchor placement and tracking
node {
  calculator: "BoxTrackingSubgraph"
  input_stream: "VIDEO:input_video"
  input_stream: "START_POS:start_pos"
  input_stream: "CANCEL_ID:cancel_object_id"
  output_stream: "BOXES:boxes"
}
```

### Box Tracking Subgraph

![box_tracking_subgraph](images/box-tracking-subgraph.png)

[Source pbtxt file](https://github.com/google/mediapipe/tree/master/mediapipe/graphs/tracking/subgraphs/box_tracking_gpu.pbtxt)

```bash
# MediaPipe box tracking subgraph.

type: "BoxTrackingSubgraph"

input_stream: "VIDEO:input_video"
input_stream: "BOXES:start_pos"
input_stream: "CANCEL_ID:cancel_object_id"
output_stream: "BOXES:boxes"

node: {
  calculator: "ImageTransformationCalculator"
  input_stream: "IMAGE_GPU:input_video"
  output_stream: "IMAGE_GPU:downscaled_input_video"
  node_options: {
    [type.googleapis.com/mediapipe.ImageTransformationCalculatorOptions] {
      output_width: 240
      output_height: 320
    }
  }
}

# Converts GPU buffer to ImageFrame for processing tracking.
node: {
  calculator: "GpuBufferToImageFrameCalculator"
  input_stream: "downscaled_input_video"
  output_stream: "downscaled_input_video_cpu"
}

# Performs motion analysis on an incoming video stream.
node: {
  calculator: "MotionAnalysisCalculator"
  input_stream: "VIDEO:downscaled_input_video_cpu"
  output_stream: "CAMERA:camera_motion"
  output_stream: "FLOW:region_flow"

  node_options: {
    [type.googleapis.com/mediapipe.MotionAnalysisCalculatorOptions]: {
      analysis_options {
        analysis_policy: ANALYSIS_POLICY_CAMERA_MOBILE
        flow_options {
          fast_estimation_min_block_size: 100
          top_inlier_sets: 1
          frac_inlier_error_threshold: 3e-3
          downsample_mode: DOWNSAMPLE_TO_INPUT_SIZE
          verification_distance: 5.0
          verify_long_feature_acceleration: true
          verify_long_feature_trigger_ratio: 0.1
          tracking_options {
            max_features: 500
            adaptive_extraction_levels: 2
            min_eig_val_settings {
              adaptive_lowest_quality_level: 2e-4
            }
            klt_tracker_implementation: KLT_OPENCV
          }
        }
      }
    }
  }
}

# Reads optical flow fields defined in
# mediapipe/framework/formats/motion/optical_flow_field.h,
# returns a VideoFrame with 2 channels (v_x and v_y), each channel is quantized
# to 0-255.
node: {
  calculator: "FlowPackagerCalculator"
  input_stream: "FLOW:region_flow"
  input_stream: "CAMERA:camera_motion"
  output_stream: "TRACKING:tracking_data"

  node_options: {
    [type.googleapis.com/mediapipe.FlowPackagerCalculatorOptions]: {
      flow_packager_options: {
        binary_tracking_data_support: false
      }
    }
  }
}

# Tracks box positions over time.
node: {
  calculator: "BoxTrackerCalculator"
  input_stream: "TRACKING:tracking_data"
  input_stream: "TRACK_TIME:input_video"
  input_stream: "START_POS:start_pos"
  input_stream: "CANCEL_OBJECT_ID:cancel_object_id"
  input_stream_info: {
    tag_index: "CANCEL_OBJECT_ID"
    back_edge: true
  }
  output_stream: "BOXES:boxes"

  input_stream_handler {
    input_stream_handler: "SyncSetInputStreamHandler"
    options {
      [mediapipe.SyncSetInputStreamHandlerOptions.ext] {
        sync_set {
          tag_index: "TRACKING"
          tag_index: "TRACK_TIME"
        }
        sync_set {
          tag_index: "START_POS"
        }
        sync_set {
          tag_index: "CANCEL_OBJECT_ID"
        }
      }
    }
  }

  node_options: {
    [type.googleapis.com/mediapipe.BoxTrackerCalculatorOptions]: {
      tracker_options: {
        track_step_options {
          track_object_and_camera: true
          tracking_degrees: TRACKING_DEGREE_OBJECT_SCALE
          inlier_spring_force: 0.0
          static_motion_temporal_ratio: 3e-2
        }
      }
      visualize_tracking_data: false
      streaming_track_data_cache_size: 100
    }
  }
}
```

## Licensing & Use

    Copyright 2020 Google LLC

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        https://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
