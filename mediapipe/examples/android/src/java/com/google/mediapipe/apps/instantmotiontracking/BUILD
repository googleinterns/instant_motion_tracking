licenses(["notice"])  # Apache 2.0

package(default_visibility = ["//visibility:private"])

android_library(
    name = "basic_lib",
    srcs = glob(["*.java"]),
    manifest = "AndroidManifest.xml",
    resource_files = glob(["res/**"]),
    visibility = ["//visibility:public"],
    deps = [
        "//mediapipe/java/com/google/mediapipe/components:android_camerax_helper",
        "//mediapipe/java/com/google/mediapipe/components:android_components",
        "//mediapipe/java/com/google/mediapipe/framework:android_framework",
        "//mediapipe/java/com/google/mediapipe/glutil",
        "//third_party:androidx_appcompat",
        "//third_party:androidx_constraint_layout",
        "//third_party:opencv",
        "@maven//:androidx_concurrent_concurrent_futures",
        "@maven//:androidx_core_core",
        "@maven//:com_google_guava_guava",
        "@maven//:com_github_bumptech_glide_glide",
        "@maven//:com_github_bumptech_glide_gifdecoder",
        "@com_google_protobuf//:protobuf_java",
    ],
)

# Include all calculators specific to this project defined by BUILD in graphs
cc_binary(
    name = "libmediapipe_jni.so",
    linkshared = 1,
    linkstatic = 1,
    deps = [
        "//mediapipe/graphs/instantmotiontracking:mobile_calculators",
        "//mediapipe/java/com/google/mediapipe/framework/jni:mediapipe_framework_jni",
    ],
)

# Converts the .so cc_binary into a cc_library, to be consumed in an android_binary.
cc_library(
    name = "mediapipe_jni_lib",
    srcs = [":libmediapipe_jni.so"],
    alwayslink = 1,
)

android_binary(
    name = "instantmotiontracking",
    assets = [
        "//mediapipe/graphs/instantmotiontracking:mobile.binarypb",
        "//mediapipe/examples/android/src/java/com/google/mediapipe/apps/instantmotiontracking/assets/gif:gif.obj.uuu",
        "//mediapipe/examples/android/src/java/com/google/mediapipe/apps/instantmotiontracking/assets/gif:default_gif_texture.jpg",
        "//mediapipe/examples/android/src/java/com/google/mediapipe/apps/instantmotiontracking/assets/robot:robot.obj.uuu",
        "//mediapipe/examples/android/src/java/com/google/mediapipe/apps/instantmotiontracking/assets/robot:robot_texture.jpg",
    ],
    assets_dir = "",
    manifest = "AndroidManifest.xml",
    manifest_values = {
        "applicationId": "com.google.mediapipe.apps.instantmotiontracking",
        "appName": "Instant Motion Tracking",
        "mainActivity": ".MainActivity",
        "cameraFacingFront": "False",
        "binaryGraphName": "mobile.binarypb",
        "inputVideoStreamName": "input_video",
        "outputVideoStreamName": "output_video",
    },
    multidex = "native",
    deps = [
        ":mediapipe_jni_lib",
        ":basic_lib",
    ],
)
