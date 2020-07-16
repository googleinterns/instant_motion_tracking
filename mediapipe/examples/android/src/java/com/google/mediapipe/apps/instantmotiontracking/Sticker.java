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

import java.util.ArrayList;
import com.google.protobuf.*;

// Stickers represent a unique object to render and manipulate in an AR scene.
// A sticker has a sticker_id (a unique integer identifying a sticker object to
// render), x and y normalized anchor coordinates [0.0-1.0], user inputs for
// rotation in radians, scaling, and a renderID (another unique integer which
// determines what object model to render for this unique sticker).

public class Sticker {
  public static int TOTAL_NUM_ASSETS = 3; // Total number of different types of
  // objects that can be rendered

  private int renderID = 0; // ID of object to render

  private float xAnchor, yAnchor; // Normalized X and Y coordinates of anchor
  // (0,0) lies at top-left corner of screen
  // (1.0,1.0) lies at bottom-right corner of screen

  private float userRotation = 0f; // Rotation in radians from user
  private float userScalingFactor = 1f; // Scaling factor as defined by user (defaults to 1.0)

  private int stickerID; // Unique sticker integer ID

  private static int globalIDLimit = 1; // Used to determine next stickerID

  public Sticker() {
    this.renderID = 0;
    stickerID = (Sticker.globalIDLimit++);
  }

  /** Sets a new asset rendering ID as defined by Mediapipe graph. * */
  public Sticker(int renderID) {
    this.renderID = renderID;
    stickerID = (Sticker.globalIDLimit++);
  }

  /** Return sticker ID integer. * */
  public int getStickerID() {
    return this.stickerID;
  }

  /** Sets new normalized X and Y anchor coordinates. * */
  public void setNewAnchor(float normalizedX, float normalizedY) {
    this.xAnchor = normalizedX;
    this.yAnchor = normalizedY;
  }

  /** Return integer array of format int[]{xAnchor,yAnchor}. * */
  public float[] getAnchor() {
    return new float[] {xAnchor, yAnchor};
  }

  /** Returns render asset ID used by Mediapipe graph. * */
  public int getRenderAssetID() {
    return renderID;
  }

  public void setRenderAssetID(int newRenderID) {
    this.renderID = newRenderID;
  }

  /** Increments/Loops the render ID in order to change the object in scene **/
  public void loopAssetID() {
    if(getRenderAssetID() >= TOTAL_NUM_ASSETS - 1) {
      setRenderAssetID(0);
    }
    else {
      setRenderAssetID(getRenderAssetID() + 1);
    }
  }

  /** Sets new user input of y-axis rotation (objective rotation, does not increment). * */
  public void setRotation(float radians) {
    this.userRotation = radians;
  }

  /** Return radians of rotation on the object set by user. * */
  public float getRotation() {
    return this.userRotation;
  }

  /** Sets new scale factor **/
  public void setScaleFactor(float scaling) {
    this.userScalingFactor = scaling;
  }

  /** Get current scale factor **/
  public float getScaleFactor() {
    return this.userScalingFactor;
  }

  /**
   * This method converts an ArrayList of stickers to a MessageLite object which
   * can be passed directly to the MediaPipe graph.
   *
   * @param stickerList ArrayList of Sticker objects to convert to data string
   * @return MessageLite protobuffer of all sticker data
   */
  public static MessageLite getMessageLiteData(ArrayList<Sticker> stickerArrayList) {
    StickerBuffer.StickerRoll.Builder stickerRollBuilder = StickerBuffer.StickerRoll.newBuilder();
    for (final Sticker sticker : stickerArrayList) {
      StickerBuffer.Sticker protoSticker = StickerBuffer.Sticker.newBuilder()
      .setId(sticker.getStickerID())
      .setX(sticker.getAnchor()[0])
      .setY(sticker.getAnchor()[1])
      .setRotation(sticker.getRotation())
      .setScale(sticker.getScaleFactor())
      .setRenderID(sticker.renderID).build();
      stickerRollBuilder.addSticker(protoSticker);
    }
    return stickerRollBuilder.build();
  }
}
