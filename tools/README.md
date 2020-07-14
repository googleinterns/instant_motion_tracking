# Using the GL-Tools Suite

This document covers the use of reformatting and object simplification tools in order to develop render files that can be used in the InstantMotionTracking project via the gl_animation_overlay_calculator.

## OpenGL with InstantMotionTracking

InstantMotionTracking uses OpenGL in order to render objects in a 3-dimensional space and orient them to give the effect of an augmented reality experience. The calculator responsible for rendering the 3d objects (gl_animation_overlay_calculator) loads a texture in bitmap (.bmp) format, as well as an object structure in a simplified object format (.obj.uuu). The following steps explain the process of converting a sequence of .obj files (an animation) or a single .obj file into the simplified format.

###### Note: Both animations (sequence of .objs) and single static objects (single .obj), are simplified to a single .obj.uuu file that can be loaded into a MediaPipe project and used by the OpenGL calculator.

#### Steps to generate .obj.uuu files

##### 1. Place the sequence of .obj files or single .obj file in input_objs/
##### 2. Run the following command to resequence the object files

Prior to simplification by SimpleObjEncryptor_deploy.jar, the .obj file must be reordered with vertices/faces/textures in a uniform order. This functionality is provided by ref.sh.

```bash
./ref.sh
```

The .obj files will be requenced and exported to output_objs/

##### 3. Run the following command to simplify the object

```bash
java -jar SimpleObjEncryptor_deploy.jar -compressed_mode=true -dir_to_encrypt=output_objs/ -output_dir=simplified_objs/
```

The simplified .obj.uuu file can be found in the corresponding directory.
