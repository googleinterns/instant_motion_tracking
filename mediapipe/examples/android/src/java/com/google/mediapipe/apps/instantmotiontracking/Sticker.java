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

import com.google.protobuf.*;
import java.util.ArrayList;

/**
 * This class represents a single sticker object placed in the
 * instantmotiontracking system. Stickers represent a unique object to render
 * and manipulate in an AR scene.
 * <p>A sticker has a sticker_id (a unique integer identifying a sticker object
 * to render), x and y normalized anchor coordinates [0.0-1.0], user inputs for
 * rotation in radians, scaling, and a renderID (another unique integer which
 * determines what object model to render for this unique sticker).
 */
public class Sticker {

  /** All types of possible objects to render for our application. */
  public enum Render {
    // Every possible render for a sticker object
    GIF,
    ASSET_1;

    /**
     * Once called, will set the value of the current render to the next
     * possible Render available. If all possible Renders have been iterated
     * through, the function will loop and set to the first available Render.
     */
    public Render iterate() {
      int newEnumIdx = (this.ordinal() + 1) % Render.values().length;
      return Render.values()[newEnumIdx];
    }
  }

  // Current render of the sticker object
  private Render currentRender;

  // Normalized X and Y coordinates of anchor
  // (0,0) lies at top-left corner of screen
  // (1.0,1.0) lies at bottom-right corner of screen
  private float anchorX;
  private float anchorY;

  // Rotation in radians from user
  private float userRotation = 0f;
  // Scaling factor as defined by user (defaults to 1.0)
  private float userScalingFactor = 1f;

  // Unique sticker integer ID
  private int stickerId;

  // Used to determine next stickerId
  private static int globalIDLimit = 1;

  /**
   * Used to create a Sticker object with a newly generated stickerId and a
   * default Render of the first possible render in our Render enum.
   */
  public Sticker() {
    // Every sticker will have a default render of the first non-GIF asset
    this.currentRender = Render.values()[1];
    // Sticker will render out of view by default
    this.setAnchorCoordinate(2.0f, 2.0f);
    // Set the global sticker ID limit for the next sticker
    stickerId = (Sticker.globalIDLimit++);
  }

  /**
   * Used to create a Sticker object with a newly generated stickerId.
   *
   * @param render initial Render of the new Sticker object
   */
  public Sticker(Render render) {
    this.currentRender = render;
    // Sticker will render out of view by default
    this.setAnchorCoordinate(2.0f, 2.0f);
    // Set the global sticker ID limit for the next sticker
    stickerId = (Sticker.globalIDLimit++);
  }

  /**
   * Used to get the sticker ID of the object.
   *
   * @return integer of the unique sticker ID
   */
  public int getstickerId() {
    return this.stickerId;
  }

  /**
   * Used to update or reset the anchor positions in normalized [0.0-1.0]
   * coordinate space for the sticker object.
   *
   * @param normalizedX normalized X coordinate for the new anchor position
   * @param normalizedY normalized Y coordinate for the new anchor position
   */
  public void setAnchorCoordinate(float normalizedX, float normalizedY) {
    this.anchorX = normalizedX;
    this.anchorY = normalizedY;
  }

  /** @return the normalized X anchor coordinate of the sticker object */
  public float getAnchorX() {
    return anchorX;
  }

  /** @return the normalized Y anchor coordinate of the sticker object */
  public float getAnchorY() {
    return anchorY;
  }

  /** @return current asset to be rendered for this sticker object */
  public Render getRender() {
    return currentRender;
  }

  /** @param render object to render for this sticker object */
  public void setRender(Render render) {
    this.currentRender = render;
  }

  /**
   * Sets new user value of rotation radians. This rotation is not cumulative,
   * and must be set to an absolute value of rotation applied to the object.
   *
   * @param radians specified radians to rotate the sticker object by
   */
  public void setRotation(float radians) {
    this.userRotation = radians;
  }

  /** @return current user radian rotation setting */
  public float getRotation() {
    return this.userRotation;
  }

  /**
   * Sets new user scale factor. This factor will be proportional to the scale
   * of the sticker object.
   *
   * @param scaling scale factor to be applied
   */
  public void setScaleFactor(float scaling) {
    this.userScalingFactor = scaling;
  }

  /** @return current user scale factor setting */
  public float getScaleFactor() {
    return this.userScalingFactor;
  }

  /**
   * This method converts an ArrayList of stickers to a MessageLite object
   * which can be passed directly to the MediaPipe graph.
   *
   * @param stickerList ArrayList of Sticker objects to convert to data string
   * @return MessageLite protobuffer of all sticker data
   */
  public static MessageLite getMessageLiteData(
    ArrayList<Sticker> stickerArrayList) {
    StickerBuffer.StickerRoll.Builder stickerRollBuilder
      = StickerBuffer.StickerRoll.newBuilder();
    for (final Sticker sticker : stickerArrayList) {
      StickerBuffer.Sticker protoSticker =
          StickerBuffer.Sticker.newBuilder()
              .setId(sticker.getstickerId())
              .setX(sticker.getAnchorX())
              .setY(sticker.getAnchorY())
              .setRotation(sticker.getRotation())
              .setScale(sticker.getScaleFactor())
              .setRenderID(sticker.getRender().ordinal())
              .build();
      stickerRollBuilder.addSticker(protoSticker);
    }
    return stickerRollBuilder.build();
  }
}
