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

import android.content.ClipDescription;
import android.content.Context;
import android.content.Intent;
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
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.util.Size;
import android.view.inputmethod.InputMethodManager;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.graphics.Color;
import android.graphics.Matrix;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.ImageView;

import androidx.appcompat.app.AppCompatActivity;
import androidx.constraintlayout.widget.ConstraintLayout;

import com.google.protobuf.MessageLite;
import com.google.mediapipe.components.CameraHelper;
import com.google.mediapipe.components.CameraXPreviewHelper;
import com.google.mediapipe.components.ExternalTextureConverter;
import com.google.mediapipe.components.FrameProcessor;
import com.google.mediapipe.components.PermissionHelper;
import com.google.mediapipe.framework.AndroidAssetUtil;
import com.google.mediapipe.framework.AndroidPacketCreator;
import com.google.mediapipe.framework.Packet;
import com.google.mediapipe.glutil.EglManager;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.bumptech.glide.Glide;
import com.bumptech.glide.gifdecoder.StandardGifDecoder;
import com.bumptech.glide.load.resource.gif.GifDrawable;
import com.bumptech.glide.request.target.SimpleTarget;
import com.bumptech.glide.request.transition.Transition;

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

  private ArrayList<Sticker> stickerArrayList;
  private Sticker currentSticker; // Sticker being edited

  // Define parameters for 'reactivity' of object
  private final float ROTATION_SPEED = 5.0f;
  private final float SCALING_FACTOR = 0.025f;

  // Parameters of device visual field for rendering system
  // (68 degrees, 4:3 for Pixel 4)
  // TODO: Make acquisition of this information automated
  private final float VERTICAL_FOV_RADIANS = (float)Math.toRadians(68.0);
  private final float ASPECT_RATIO = (3.0f/4.0f);
  private final String FOV_SIDE_PACKET_TAG = "vertical_fov_radians";
  private final String ASPECT_RATIO_SIDE_PACKET_TAG = "aspect_ratio";

  private float[] imuData = new float[3]; // [roll,pitch,yaw]

  private static final String STICKER_PROTO_TAG = "sticker_proto_string";
  private static final String IMU_DATA_TAG = "imu_data";
  // Assets for object rendering
  // All robot animation assets and tags
  // TODO: Grouping all tags and assets into a seperate structure
  // TODO: bitmaps are space heavy, try to use compressed like png/webp
  private Bitmap robotTexture = null;
  private static final String ROBOT_TEXTURE = "robot_texture.bmp";
  private static final String ROBOT_FILE = "robot.obj.uuu";
  private static final String ROBOT_TEXTURE_TAG = "robot_texture";
  private static final String ROBOT_ASSET_TAG = "robot_asset_name";
  // All dino animation assets and tags
  private Bitmap dinoTexture = null;
  private static final String DINO_TEXTURE = "dino_texture.bmp";
  private static final String DINO_FILE = "dino.obj.uuu";
  private static final String DINO_TEXTURE_TAG = "dino_texture";
  private static final String DINO_ASSET_TAG = "dino_asset_name";
  // All GIF animation assets and tags
  private GIFEditText editText;
  private static final String GOOFY_TEST_GIF =
    "http://media.tenor.com/images/bce26b3402f8c22452fb648ee2276ff2/tenor.gif";
  private ArrayList<Bitmap> GIFBitmaps = new ArrayList<Bitmap>();
  private int gifCurrentIndex = 0;
  private static final int GIF_FRAME_RATE = 20; // 20 FPS
  // last time the GIF was updated
  private long gifLastFrameUpdateMS = System.currentTimeMillis();
  private Bitmap defaultGIFTexture = null; // Texture sent if no gif available
  private static final String DEFAULT_GIF_TEXTURE = "default_gif_texture.bmp";
  private static final String GIF_FILE = "gif.obj.uuu";
  private static final String GIF_TEXTURE_TAG = "gif_texture";
  private static final String GIF_ASSET_TAG = "gif_asset_name";

  @Override
  protected void onCreate(Bundle savedInstanceState) {

    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);

    editText = findViewById(R.id.gif_edit_text);
    editText.setOnRichContentListener(new GIFEditText.OnRichContentListener() {
        @Override
        public void onRichContent(Uri contentUri, ClipDescription description) {
          // The application must have permission to access the GIF content URI
          grantUriPermission("com.google.mediapipe.apps.instantmotiontracking",
            contentUri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
          // Set GIF frames from content URI
          setGIFBitmaps(contentUri.toString());
          // Close the keyboard upon GIF acquisition
          closeKeyboard();
        }
    });

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

    // Load a basic GIF for initialization of GIF assets
    setGIFBitmaps(GOOFY_TEST_GIF);
    prepareDemoAssets();
    AndroidPacketCreator packetCreator = processor.getPacketCreator();

    // TODO: Should come from querying the video frame
    Map<String, Packet> inputSidePackets = new HashMap<>();
    inputSidePackets.put(ROBOT_TEXTURE_TAG, packetCreator.createRgbaImageFrame(robotTexture));
    inputSidePackets.put(ROBOT_ASSET_TAG, packetCreator.createString(ROBOT_FILE));
    inputSidePackets.put(DINO_TEXTURE_TAG, packetCreator.createRgbaImageFrame(dinoTexture));
    inputSidePackets.put(DINO_ASSET_TAG, packetCreator.createString(DINO_FILE));
    inputSidePackets.put(GIF_ASSET_TAG, packetCreator.createString(GIF_FILE));
    processor.setInputSidePackets(inputSidePackets);

    // Add frame listener to PacketManagement system
    mediaPipePacketManager = new MediaPipePacketManager();
    processor.setOnWillAddFrameListener(mediaPipePacketManager);

    // Send device properties to render objects via OpenGL
    Map<String, Packet> devicePropertiesSidePackets = new HashMap<>();
    devicePropertiesSidePackets.put(ASPECT_RATIO_SIDE_PACKET_TAG, packetCreator.createFloat32(ASPECT_RATIO));
    devicePropertiesSidePackets.put(FOV_SIDE_PACKET_TAG, packetCreator.createFloat32(VERTICAL_FOV_RADIANS));
    processor.setInputSidePackets(devicePropertiesSidePackets);

    // Begin with 0 stickers in dataset
    stickerArrayList = new ArrayList<Sticker>();
    currentSticker = null;

    // TODO: Change to use ROTATION_VECTOR or non-deprecated IMU method
    SensorManager sensorManager = (SensorManager) getSystemService(SENSOR_SERVICE);
    List sensorList = sensorManager.getSensorList(Sensor.TYPE_ORIENTATION);
    sensorManager.registerListener(new SensorEventListener() {

      public void onAccuracyChanged(Sensor sensor, int accuracy) {}
      // Update procedure on sensor adjustment (phone changes orientation)
      public void onSensorChanged(SensorEvent event) {
        // All values must be negated in order to adjust the object orientations
        // to match the phone movement (passed into MatricesManagerCalculator)
        imuData[0] = -(float) Math.toRadians(event.values[0]);
        imuData[1] = -(float) Math.toRadians(event.values[1]);
        imuData[2] = -(float) Math.toRadians(event.values[2]);
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
              float newScaleFactor = getNewScaleFactor(event, currentSticker.getScaleFactor());
              currentSticker.setScaleFactor(newScaleFactor);
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
  public float getNewScaleFactor(MotionEvent event, float currentScaleFactor) {
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
                    ? -SCALING_FACTOR
                    : SCALING_FACTOR; // Are they moving towards each other?
    currentScaleFactor *= (1f + sign_float);
    return currentScaleFactor;
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
                    stickerArrayList.remove(currentSticker);
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
        // Change sticker to next possible render
        ImageButton loopRender = new ImageButton(this);
        setUIControlButtonDesign(loopRender, R.drawable.baseline_loop_24);
        loopRender.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                currentSticker.loopAssetID();
                refreshUI();
            }
        });
        buttonLayout.addView(deleteSticker);
        buttonLayout.addView(goBack);
        buttonLayout.addView(loopRender);

        // Add the GIF search option if current sticker is GIF
        if(currentSticker.getRenderAssetID() == Sticker.GIF) {
          ImageButton gifSearch = new ImageButton(this);
          setUIControlButtonDesign(gifSearch, R.drawable.baseline_search_24);
          gifSearch.setOnClickListener(new View.OnClickListener() {
              public void onClick(View v) {
                // Clear the text field to prevent text artifacts in GIF selection
                editText.setText("");
                // Open the Keyboard to allow user input
                openKeyboard();
              }
          });
          buttonLayout.addView(gifSearch);
        }
    }
    else {
        buttonLayout.removeAllViews();
        // Display stickers
        for(final Sticker sticker : stickerArrayList) {
            final ImageButton stickerButton = new ImageButton(this);
            stickerButton.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    currentSticker = sticker;
                    refreshUI();
                }
            });
            if(sticker.getRenderAssetID() == Sticker.ROBOT) {
              setStickerButtonDesign(stickerButton, R.drawable.robot);
            }
            else if(sticker.getRenderAssetID() == Sticker.DINO) {
              setStickerButtonDesign(stickerButton, R.drawable.dino);
            }
            else if(sticker.getRenderAssetID() == Sticker.GIF) {
              setUIControlButtonDesign(stickerButton, R.drawable.baseline_gif_24);
            }

            buttonLayout.addView(stickerButton);
        }
        ImageButton addSticker = new ImageButton(this);
        setUIControlButtonDesign(addSticker, R.drawable.baseline_add_24);
        addSticker.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                stickerArrayList.add(new Sticker());
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
      btn.setLayoutParams(new LinearLayout.LayoutParams(200, 200));
      btn.setPadding(25, 25, 25, 25);
      btn.setScaleType(ImageView.ScaleType.FIT_XY);
  }

  // Sets ImageButton UI for Sticker Buttons
  public void setStickerButtonDesign(ImageButton btn, int imageDrawable) {
      btn.setImageDrawable(getResources().getDrawable(imageDrawable));
      btn.setBackground(getResources().getDrawable(R.drawable.circle_button));
      btn.setLayoutParams(new LinearLayout.LayoutParams(250, 250));
      btn.setPadding(25, 25, 25, 25);
      btn.setScaleType(ImageView.ScaleType.CENTER_INSIDE);
  }

  public void setGIFBitmaps(String gif_url) {
    GIFBitmaps = new ArrayList<Bitmap>(); // Empty the bitmap array
    Glide.with(this).asGif().load(gif_url).into(new SimpleTarget<GifDrawable>() {
      @Override
      public void onResourceReady(GifDrawable resource, Transition<? super GifDrawable> transition) {
        try {
          Object startConstant = resource.getConstantState();
          Field frameManager = startConstant.getClass().getDeclaredField("frameLoader");
          frameManager.setAccessible(true);
          Object frameLoader = frameManager.get(startConstant);

          Field decoder = frameLoader.getClass().getDeclaredField("gifDecoder");
          decoder.setAccessible(true);
          StandardGifDecoder GIFDecoder = (StandardGifDecoder) decoder.get(frameLoader);
          for (int i = 0; i < GIFDecoder.getFrameCount(); i++) {
            GIFDecoder.advance();
            Bitmap bmp = GIFDecoder.getNextFrame();
            // Bitmaps must be flipped due to native acquisition of frames from Android OS
            Matrix matrix = new Matrix();
            matrix.preScale(-1.0f, 1.0f);
            GIFBitmaps.add(Bitmap.createBitmap(bmp, 0, 0, bmp.getWidth(),
              bmp.getHeight(), matrix, true));
          }
        }
        catch (Exception e) {
          e.printStackTrace();
        }
      }
    });
  }

  // Function that is continuously called in order to time GIF frame updates
  public void updateGIFFrame() {
    long millisPerFrame = 1000/GIF_FRAME_RATE;
    if(System.currentTimeMillis() - gifLastFrameUpdateMS >= millisPerFrame) {
      // Update GIF timestamp
      gifLastFrameUpdateMS = System.currentTimeMillis();
      // Cycle through every possible frame and avoid a divide by 0
      gifCurrentIndex = (gifCurrentIndex + 1) % ((GIFBitmaps.size() > 0)?(GIFBitmaps.size()):(1));
    }
  }

  // Called once to popup the Keyboard via Android OS with focus set to editText
  public void openKeyboard() {
      editText.requestFocus();
      InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
      imm.showSoftInput(editText, InputMethodManager.SHOW_IMPLICIT);
  }

  // Called once to close the Keyboard via Android OS
  public void closeKeyboard() {
      View view = this.getCurrentFocus();
      if (view != null) {
          InputMethodManager imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
          imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
      }
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
      InputStream inputStream = getAssets().open(ROBOT_TEXTURE);
      robotTexture = BitmapFactory.decodeStream(inputStream, null /*outPadding*/, decodeOptions);
      inputStream.close();
    } catch (Exception e) {
      Log.e(TAG, "Error parsing object texture; error: " + e);
      throw new IllegalStateException(e);
    }

    try {
      InputStream inputStream = getAssets().open(DINO_TEXTURE);
      dinoTexture = BitmapFactory.decodeStream(inputStream, null /*outPadding*/, decodeOptions);
      inputStream.close();
    } catch (Exception e) {
      Log.e(TAG, "Error parsing object texture; error: " + e);
      throw new IllegalStateException(e);
    }

    try {
      InputStream inputStream = getAssets().open(DEFAULT_GIF_TEXTURE);
      defaultGIFTexture = BitmapFactory.decodeStream(inputStream, null /*outPadding*/, decodeOptions);
      inputStream.close();
    } catch (Exception e) {
      Log.e(TAG, "Error parsing object texture; error: " + e);
      throw new IllegalStateException(e);
    }
  }

  private class MediaPipePacketManager implements FrameProcessor.OnWillAddFrameListener {
    @Override
    public void onWillAddFrame(long timestamp) {
      // set current GIF bitmap as default texture
      Bitmap currentGIFBitmap = defaultGIFTexture;
      // If current index is in bounds, display current frame
      if(gifCurrentIndex <= GIFBitmaps.size() - 1)
        currentGIFBitmap = GIFBitmaps.get(gifCurrentIndex);
      // Update to next GIF frame based on timing and frame rate
      updateGIFFrame();

      // Initialize sticker data protobuffer packet information
      Packet stickerProtoDataPacket = processor.getPacketCreator().createSerializedProto(Sticker.getMessageLiteData(stickerArrayList));
      // Define and set the IMU sensory information float array
      Packet imuDataPacket = processor.getPacketCreator().createFloat32Array(imuData);
      // Communicate GIF textures (dynamic texturing) to graph
      Packet gifTexturePacket = processor.getPacketCreator().createRgbaImageFrame(currentGIFBitmap);
      processor.getGraph().addConsumablePacketToInputStream(STICKER_PROTO_TAG, stickerProtoDataPacket, timestamp);
      processor.getGraph().addConsumablePacketToInputStream(IMU_DATA_TAG, imuDataPacket, timestamp);
      processor.getGraph().addConsumablePacketToInputStream(GIF_TEXTURE_TAG, gifTexturePacket, timestamp);
      stickerProtoDataPacket.release();
      imuDataPacket.release();
      gifTexturePacket.release();
    }
  }
}
