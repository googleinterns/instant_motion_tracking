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

package com.google.mediapipe.apps.instantmotiontracking;

import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.SurfaceTexture;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.util.Log;
import android.util.Size;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import androidx.appcompat.app.AppCompatActivity;
import androidx.constraintlayout.widget.ConstraintLayout;
import com.google.mediapipe.components.CameraHelper;
import com.google.mediapipe.components.CameraXPreviewHelper;
import com.google.mediapipe.components.ExternalTextureConverter;
import com.google.mediapipe.components.FrameProcessor;
import com.google.mediapipe.components.PermissionHelper;
import com.google.mediapipe.framework.AndroidAssetUtil;
import com.google.mediapipe.framework.AndroidPacketCreator;
import com.google.mediapipe.framework.Packet;
import com.google.mediapipe.glutil.EglManager;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class MainActivity extends AppCompatActivity {
  private static final String TAG = "MainActivity";
  private static final boolean FLIP_FRAMES_VERTICALLY = true;

  static {
    System.loadLibrary("mediapipe_jni");
    try {
      System.loadLibrary("opencv_java3");
    } catch (java.lang.UnsatisfiedLinkError e) {
      System.loadLibrary("opencv_java4");
    }
  }

  protected FrameProcessor processor;
  protected CameraXPreviewHelper cameraHelper;
  private SurfaceTexture previewFrameTexture;
  private SurfaceView previewDisplayView;
  private EglManager eglManager;
  private ExternalTextureConverter converter;
  private ApplicationInfo applicationInfo;

  // Allows for automated packet transmission to graph
  private MediapipePacketManager mPacketManager;

  // Bounds for a single click (sticker anchor reset)
  private final long CLICK_DURATION = 300; // ms
  private long clickStartMillis = 0;
  private ConstraintLayout constraintLayout;

  // Sticker data management
  ArrayList<Sticker> stickerList = new ArrayList<Sticker>();
  // sticker_focus_value is index of sticker to update anchor positions for
  int sticker_focus_value = -1;
  // Current implementation uses one sticker
  Sticker currentSticker;
  // [roll,pitch,yaw] from IMU sensors
  private float[] IMUData = new float[3];

  // Define procedure for IMUData update
  SensorManager sensorManager = null;
  SensorEventListener sensorListener =
      new SensorEventListener() {
        public void onAccuracyChanged(Sensor sensor, int accuracy) {}

        public void onSensorChanged(SensorEvent event) {
          IMUData[0] = (float) Math.toRadians((event.values[1] + 180.0));
          IMUData[1] = (float) Math.toRadians(event.values[2]);
          IMUData[2] = (float) Math.toRadians(event.values[0]);
        }
      };

  // Assets for object rendering
  private Bitmap objTexture = null;
  private Bitmap boxTexture = null;
  private static final String BOX_TEXTURE = "texture.bmp";
  private static final String BOX_FILE = "model.obj.uuu";
  private static final String OBJ_TEXTURE = "classic_colors.png";
  private static final String OBJ_FILE = "box.obj.uuu";

  @Override
  protected void onCreate(Bundle savedInstanceState) {

    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);

    stickerList.add(new Sticker());
    currentSticker = stickerList.get(0);

    // Define sensor properties (only get one orientation sensor)
    sensorManager = (SensorManager) getSystemService(SENSOR_SERVICE);
    List sensorList = sensorManager.getSensorList(Sensor.TYPE_ORIENTATION);
    sensorManager.registerListener(
        sensorListener, (Sensor) sensorList.get(0), SensorManager.SENSOR_DELAY_FASTEST);

    try {
      applicationInfo =
          getPackageManager().getApplicationInfo(getPackageName(), PackageManager.GET_META_DATA);
    } catch (NameNotFoundException e) {
      Log.e(TAG, "Cannot find application info: " + e);
    }

    previewDisplayView = new SurfaceView(this);
    setupPreviewDisplayView();

    // Initialize asset manager so that MediaPipe native libraries can access the app assets, e.g.,
    // binary graphs.
    AndroidAssetUtil.initializeNativeAssetManager(this);
    eglManager = new EglManager(null);
    processor =
        new FrameProcessor(
            this,
            eglManager.getNativeContext(),
            applicationInfo.metaData.getString("binaryGraphName"),
            applicationInfo.metaData.getString("inputVideoStreamName"),
            applicationInfo.metaData.getString("outputVideoStreamName"));
    processor.getVideoSurfaceOutput().setFlipY(FLIP_FRAMES_VERTICALLY);
    PermissionHelper.checkAndRequestCameraPermissions(this);

    // Send loaded 3d render assets as side packets to graph
    prepareDemoAssets();
    AndroidPacketCreator packetCreator = processor.getPacketCreator();
    Map<String, Packet> inputSidePackets = new HashMap<>();
    inputSidePackets.put("obj_asset_name", packetCreator.createString(OBJ_FILE));
    inputSidePackets.put("box_asset_name", packetCreator.createString(BOX_FILE));
    inputSidePackets.put("obj_texture", packetCreator.createRgbaImageFrame(objTexture));
    inputSidePackets.put("box_texture", packetCreator.createRgbaImageFrame(boxTexture));
    processor.setInputSidePackets(inputSidePackets);

    // Add frame listener to PacketManagement system
    mPacketManager = new MediapipePacketManager();
    processor.setOnWillAddFrameListener(mPacketManager);

    // Mechanisms for zoom, pinch, rotation, tap gestures (Basic single object manipulation)
    constraintLayout = (ConstraintLayout) findViewById(R.id.constraint_layout);
    constraintLayout.setOnTouchListener(
        new ConstraintLayout.OnTouchListener() {
          public boolean onTouch(View v, MotionEvent event) {
            switch (event.getAction()) {
                // Detecting a single click for object re-anchoring
              case (MotionEvent.ACTION_DOWN):
                clickStartMillis = System.currentTimeMillis();
                break;
              case (MotionEvent.ACTION_UP):
                if (System.currentTimeMillis() - clickStartMillis <= CLICK_DURATION) {
                  float x = (event.getX() / constraintLayout.getWidth());
                  float y = (event.getY() / constraintLayout.getHeight());
                  currentSticker.setNewAnchor(x, y);
                  sticker_focus_value = currentSticker.getStickerID();
                }
                break;
                // Detecting a rotation or pinch/zoom gesture to manipulate the object
              case (MotionEvent.ACTION_MOVE):
                if (event.getPointerCount() == 2) {
                  if (event.getHistorySize() > 1) {
                    // Calculate objectScaling for dynamic scaling of rendered object
                    float scaling_speed = 0.05f;
                    double new_distance =
                        distance(event.getX(0), event.getY(0), event.getX(1), event.getY(1));
                    double old_distance =
                        distance(
                            event.getHistoricalX(0, 0),
                            event.getHistoricalY(0, 0),
                            event.getHistoricalX(1, 0),
                            event.getHistoricalY(1, 0));
                    float sign_float =
                        (new_distance < old_distance)
                            ? -scaling_speed
                            : scaling_speed; // Are they moving towards each other?
                    double dist1 =
                        distance(
                            event.getX(0),
                            event.getY(0),
                            event.getHistoricalX(0, 0),
                            event.getHistoricalY(0, 0));
                    double dist2 =
                        distance(
                            event.getX(1),
                            event.getY(1),
                            event.getHistoricalX(1, 0),
                            event.getHistoricalY(1, 0));
                    float scalingIncrement = sign_float * (float) (dist1 + dist2);
                    currentSticker.setScaling(currentSticker.getScaling() + scalingIncrement);

                    // calculate objectRotation (in degrees) for dynamic y-axis rotations
                    float rotation_speed = 5.0f;
                    float tangentA =
                        (float)
                            Math.atan2(
                                event.getY(1) - event.getY(0), event.getX(1) - event.getX(0));
                    float tangentB =
                        (float)
                            Math.atan2(
                                event.getHistoricalY(1, 0) - event.getHistoricalY(0, 0),
                                event.getHistoricalX(1, 0) - event.getHistoricalX(0, 0));
                    float angle = ((float) Math.toDegrees(tangentA - tangentB)) % 360f;
                    angle += ((angle < -180f) ? (+360f) : ((angle > 180f) ? -360f : 0.0f));
                    float rotationIncrement = (float) (3.14 * ((angle * rotation_speed) / 180));
                    currentSticker.setRotation(currentSticker.getRotation() + rotationIncrement);
                  }
                }
                break;
            }
            return true;
          }
        });
  }

  public double distance(double x1, double y1, double x2, double y2) {
    return Math.sqrt((y2 - y1) * (y2 - y1) + (x2 - x1) * (x2 - x1));
  }

  @Override
  protected void onResume() {
    super.onResume();
    converter = new ExternalTextureConverter(eglManager.getContext());
    converter.setFlipY(FLIP_FRAMES_VERTICALLY);
    converter.setConsumer(processor);
    if (PermissionHelper.cameraPermissionsGranted(this)) {
      startCamera();
    }
  }

  @Override
  protected void onPause() {
    super.onPause();
    converter.close();
  }

  @Override
  public void onRequestPermissionsResult(
      int requestCode, String[] permissions, int[] grantResults) {
    super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    PermissionHelper.onRequestPermissionsResult(requestCode, permissions, grantResults);
  }

  protected void onCameraStarted(SurfaceTexture surfaceTexture) {
    previewFrameTexture = surfaceTexture;
    // Make the display view visible to start showing the preview. This triggers the
    // SurfaceHolder.Callback added to (the holder of) previewDisplayView.
    previewDisplayView.setVisibility(View.VISIBLE);
  }

  public void startCamera() {
    cameraHelper = new CameraXPreviewHelper();
    cameraHelper.setOnCameraStartedListener(
        surfaceTexture -> {
          onCameraStarted(surfaceTexture);
        });
    CameraHelper.CameraFacing cameraFacing =
        applicationInfo.metaData.getBoolean("cameraFacingFront", false)
            ? CameraHelper.CameraFacing.FRONT
            : CameraHelper.CameraFacing.BACK;
    cameraHelper.startCamera(this, cameraFacing, /*surfaceTexture=*/ null);
  }

  private void setupPreviewDisplayView() {
    previewDisplayView.setVisibility(View.GONE);
    ViewGroup viewGroup = findViewById(R.id.preview_display_layout);
    viewGroup.addView(previewDisplayView);

    previewDisplayView
        .getHolder()
        .addCallback(
            new SurfaceHolder.Callback() {
              @Override
              public void surfaceCreated(SurfaceHolder holder) {
                processor.getVideoSurfaceOutput().setSurface(holder.getSurface());
              }

              @Override
              public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                // (Re-)Compute the ideal size of the camera-preview display (the area that the
                // camera-preview frames get rendered onto, potentially with scaling and rotation)
                // based on the size of the SurfaceView that contains the display.
                Size viewSize = new Size(width, height);
                Size displaySize = cameraHelper.computeDisplaySizeFromViewSize(viewSize);
                boolean isCameraRotated = cameraHelper.isCameraRotated();

                // Connect the converter to the camera-preview frames as its input (via
                // previewFrameTexture), and configure the output width and height as the computed
                // display size.
                converter.setSurfaceTextureAndAttachToGLContext(
                    previewFrameTexture,
                    isCameraRotated ? displaySize.getHeight() : displaySize.getWidth(),
                    isCameraRotated ? displaySize.getWidth() : displaySize.getHeight());
              }

              @Override
              public void surfaceDestroyed(SurfaceHolder holder) {
                processor.getVideoSurfaceOutput().setSurface(null);
              }
            });
  }

  private void prepareDemoAssets() {
    AndroidAssetUtil.initializeNativeAssetManager(this);
    // We render from raw data with openGL, so disable decoding preprocessing
    BitmapFactory.Options decodeOptions = new BitmapFactory.Options();
    decodeOptions.inScaled = false;
    decodeOptions.inDither = false;
    decodeOptions.inPremultiplied = false;

    try {
      InputStream inputStream = getAssets().open(OBJ_TEXTURE);
      objTexture = BitmapFactory.decodeStream(inputStream, null /*outPadding*/, decodeOptions);
      inputStream.close();
    } catch (Exception e) {
      Log.e(TAG, "Error parsing object texture; error: " + e);
      throw new IllegalStateException(e);
    }

    try {
      InputStream inputStream = getAssets().open(BOX_TEXTURE);
      boxTexture = BitmapFactory.decodeStream(inputStream, null /*outPadding*/, decodeOptions);
      inputStream.close();
    } catch (Exception e) {
      Log.e(TAG, "Error parsing box texture; error: " + e);
      throw new RuntimeException(e);
    }
  }

  private class MediapipePacketManager implements FrameProcessor.OnWillAddFrameListener {
    @Override
    public void onWillAddFrame(long timestamp) {
      Packet stickerInFocusPacket =
          processor
              .getPacketCreator()
              .createInt32(
                  sticker_focus_value); // -1 if no sticker, else, sticker_id to change anchor
      sticker_focus_value = -1; // Reset the sticker focus after values have been updated
      Packet stickerDataPacket =
          processor.getPacketCreator().createString(Sticker.stickerArrayListToRawData(stickerList));
      Packet IMUDataPacket = processor.getPacketCreator().createFloat32Array(IMUData);
      processor
          .getGraph()
          .addConsumablePacketToInputStream("sticker_in_focus", stickerInFocusPacket, timestamp);
      processor
          .getGraph()
          .addConsumablePacketToInputStream("sticker_data_string", stickerDataPacket, timestamp);
      processor.getGraph().addConsumablePacketToInputStream("imu_data", IMUDataPacket, timestamp);
      stickerInFocusPacket.release();
      stickerDataPacket.release();
      IMUDataPacket.release();
    }
  }
}
