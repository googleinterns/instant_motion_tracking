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
import android.widget.Button;
import android.graphics.Color;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.ImageView;

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
  private MediaPipePacketManager mediaPipePacketManager;

  // Bounds for a single click (sticker anchor reset)
  private final long CLICK_DURATION = 300; // ms
  private long clickStartMillis = 0;
  private ConstraintLayout constraintLayout;
  // Contains dynamic layout of sticker data controller
  private LinearLayout buttonLayout;

  private ArrayList<Sticker> stickerList;
  private Sticker currentSticker; // Sticker being edited

  // Define parameters for 'reactivity' of object
  private final float ROTATION_SPEED = 5.0f;
  private final float SCALING_SPEED = 0.05f;

  private float[] imuData = new float[3]; // [roll,pitch,yaw]

  // Assets for object rendering
  private Bitmap objTexture = null;
  private static final String OBJ_TEXTURE = "chair_texture.bmp";
  private static final String OBJ_FILE = "chair.obj.uuu";
  // Tags for the side packets of model texture and .obj.uuu
  private static final String SIDE_PACKET_ASSET_TAG = "obj_asset_name";
  private static final String SIDE_PACKET_TEXTURE_TAG = "obj_texture";

  @Override
  protected void onCreate(Bundle savedInstanceState) {

    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);

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
    inputSidePackets.put(SIDE_PACKET_ASSET_TAG, packetCreator.createString(OBJ_FILE));
    inputSidePackets.put(SIDE_PACKET_TEXTURE_TAG, packetCreator.createRgbaImageFrame(objTexture));
    processor.setInputSidePackets(inputSidePackets);

    // Add frame listener to PacketManagement system
    mediaPipePacketManager = new MediaPipePacketManager();
    processor.setOnWillAddFrameListener(mediaPipePacketManager);

    // Begin with 0 stickers in dataset
    stickerList = new ArrayList<Sticker>();
    currentSticker = null;

    // Define sensor properties (only get one orientation sensor)
    SensorManager sensorManager = (SensorManager) getSystemService(SENSOR_SERVICE);
    List sensorList = sensorManager.getSensorList(Sensor.TYPE_ROTATION_VECTOR);
    sensorManager.registerListener(new SensorEventListener() {

      float[] orientation = new float[5];
      float[] mRotationMatrix = new float[16];

      public void onAccuracyChanged(Sensor sensor, int accuracy) {}
      // Update procedure on sensor adjustment (phone changes orientation)
      public void onSensorChanged(SensorEvent event) {
        SensorManager.getRotationMatrixFromVector(mRotationMatrix, event.values);
        SensorManager.getOrientation(mRotationMatrix, orientation);

        // Optionally convert the result from radians to degrees
        //orientation[0] = (float) Math.toDegrees(orientation[0]);
        //orientation[1] = (float) Math.toDegrees(orientation[1]);
        //orientation[2] = (float) Math.toDegrees(orientation[2]);

        imuData[0] = orientation[0];//yaw (float) Math.toRadians(event.values[1]);
        imuData[1] = orientation[1];//pitch [-pi,pi] (float) Math.toRadians(event.values[2]);
        imuData[2] = orientation[2];//roll (float) Math.toRadians(event.values[0]);
      }
    }, (Sensor) sensorList.get(0), SensorManager.SENSOR_DELAY_FASTEST);

    // Mechanisms for zoom, pinch, rotation, tap gestures (Basic single object manipulation
    buttonLayout = (LinearLayout) findViewById(R.id.button_layout);
    constraintLayout = (ConstraintLayout) findViewById(R.id.constraint_layout);
    constraintLayout.setOnTouchListener(
      new ConstraintLayout.OnTouchListener() {
        public boolean onTouch(View v, MotionEvent event) {
          return UITouchManager(event);
        }
      });
    refreshUI();
  }

  // Use MotionEvent properties to interpret taps/rotations/scales
  public boolean UITouchManager(MotionEvent event) {
    if(currentSticker != null) {
      switch (event.getAction()) {
        // Detecting a single click for object re-anchoring
        case (MotionEvent.ACTION_DOWN):
          clickStartMillis = System.currentTimeMillis();
          break;
        case (MotionEvent.ACTION_UP):
          if (System.currentTimeMillis() - clickStartMillis <= CLICK_DURATION) { recordClick(event); }
          break;
        case (MotionEvent.ACTION_MOVE):
          // Rotation and Scaling are independent events and can occur simulataneously
          if (event.getPointerCount() == 2) {
            if (event.getHistorySize() > 1) {
              // Calculate user scaling of sticker
              float scalingIncrement = calculateScalingDistance(event);
              currentSticker.setScaling(currentSticker.getScaling() + scalingIncrement);
              // calculate rotation (radians) for dynamic y-axis rotations
              float rotationIncrement = calculateRotationRadians(event);
              currentSticker.setRotation(currentSticker.getRotation() + rotationIncrement);
            }
          }
          break;
      }
    }
    return true;
  }

  // If our fingers are moved in a rotation, then our rotation factor will change
  // by the sum of the rotation of both fingers in radians
  public float calculateRotationRadians(MotionEvent event) {
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
    float rotationIncrement = (float) (Math.PI * ((angle * ROTATION_SPEED) / 180));
    return rotationIncrement;
  }

  // If our finger-to-finger distance increased, so will our scaling.
  // The amount is determined by the sum of the distances moved by both fingers
  public float calculateScalingDistance(MotionEvent event) {
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
                    ? -SCALING_SPEED
                    : SCALING_SPEED; // Are they moving towards each other?
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
    return scalingIncrement;
  }

  public void recordClick(MotionEvent event) {
    float x = (event.getX() / constraintLayout.getWidth());
    float y = (event.getY() / constraintLayout.getHeight());
    currentSticker.setNewAnchor(x, y);
  }

  public double distance(double x1, double y1, double x2, double y2) {
    return Math.sqrt((y2 - y1) * (y2 - y1) + (x2 - x1) * (x2 - x1));
  }

  // Called whenever a button is clicked
  public void refreshUI() {
    if(currentSticker != null) { // No sticker in view
        buttonLayout.removeAllViews();
        ImageButton deleteSticker = new ImageButton(this);
        setUIControlButtonDesign(deleteSticker, R.drawable.baseline_clear_24);
        deleteSticker.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if(currentSticker != null) {
                    stickerList.remove(currentSticker);
                    currentSticker = null;
                    refreshUI();
                }
            }
        });
        // Go to home sticker menu
        ImageButton goBack = new ImageButton(this);
        setUIControlButtonDesign(goBack, R.drawable.baseline_arrow_back_24);
        goBack.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                currentSticker = null;
                refreshUI();
            }
        });

        buttonLayout.addView(deleteSticker);
        buttonLayout.addView(goBack);
    }
    else {
        buttonLayout.removeAllViews();
        // Display stickers
        for(final Sticker sticker : stickerList) {
            final ImageButton stickerButton = new ImageButton(this);
            stickerButton.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    currentSticker = sticker;
                    refreshUI();
                }
            });
            setStickerButtonDesign(stickerButton, R.drawable.chair_preview);
            buttonLayout.addView(stickerButton);
        }
        ImageButton addSticker = new ImageButton(this);
        setUIControlButtonDesign(addSticker, R.drawable.baseline_add_24);
        addSticker.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                stickerList.add(new Sticker());
                refreshUI();
            }
        });

        buttonLayout.addView(addSticker);
    }
  }

  // Sets ImageButton UI for Control Buttons (Delete, Add, Back)
    public void setUIControlButtonDesign(ImageButton btn, int imageDrawable) {
        btn.setImageDrawable(getResources().getDrawable(imageDrawable));
        btn.setBackgroundColor(Color.parseColor("#00ffffff"));
        btn.setLayoutParams(new LinearLayout.LayoutParams(200,200));
        btn.setPadding(25,25,25,25);
        btn.setScaleType(ImageView.ScaleType.FIT_XY);
    }

    // Sets ImageButton UI for Sticker Buttons
    public void setStickerButtonDesign(ImageButton btn, int imageDrawable) {
        btn.setImageDrawable(getResources().getDrawable(imageDrawable));
        btn.setBackground(getResources().getDrawable(R.drawable.circle_button));
        btn.setLayoutParams(new LinearLayout.LayoutParams(250,250));
        btn.setPadding(25,25,25,25);
        btn.setScaleType(ImageView.ScaleType.CENTER_INSIDE);
    }

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
  }

  private class MediaPipePacketManager implements FrameProcessor.OnWillAddFrameListener {
    @Override
    public void onWillAddFrame(long timestamp) {
      Packet stickerDataPacket = processor.getPacketCreator().createString(Sticker.stickerArrayListToRawData(stickerList));
      Packet imuDataPacket = processor.getPacketCreator().createFloat32Array(imuData);
      processor.getGraph().addConsumablePacketToInputStream("sticker_data_string", stickerDataPacket, timestamp);
      processor.getGraph().addConsumablePacketToInputStream("imu_data", imuDataPacket, timestamp);
      stickerDataPacket.release();
      imuDataPacket.release();
    }
  }
}
