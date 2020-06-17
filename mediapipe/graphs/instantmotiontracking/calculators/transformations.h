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

// Radians by which to rotate the object (Provided by UI input)
typedef struct UserRotation {
   float radians;
   int sticker_id;
};

// A scaling increment provided by the UI application end
typedef struct UserScaling {
   float scaling_increment;
   int sticker_id;
};

// The normalized anchor coordinates of a sticker
typedef struct Anchor {
   float x; // [0.0-1.0]
   float y; // [0.0-1.0]
   float z; // Centered around 1.0 [current_scale = z * initial_scale]
   int sticker_id;
};

// Used to determine which object to render in overlay calculator
typedef struct RenderDescriptor {
   int render_object_id;
   int sticker_id;
};

// Representative of a translation transformation in OpenGL space
typedef struct Translation {
   float x;
   float y;
   float z;
   int sticker_id;
};

// Representative of a rotation transformation in OpenGL space
typedef struct Rotation {
   float x_radians;
   float y_radians;
   float z_radians;
   int sticker_id;
};