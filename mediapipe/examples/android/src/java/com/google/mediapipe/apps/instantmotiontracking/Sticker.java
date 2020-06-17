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

public class Sticker {

  private int renderID = 0;
  private float xAnchor, yAnchor;
  private float userRotation, userScaling;
  private int stickerID;
  private static int globalIDLimit = 1;

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

  /** Sets new user input of y-axis rotation (objective rotation, does not increment). * */
  public void setRotation(float radians) {
    this.userRotation = radians;
  }

  /** Return radians of rotation on the object set by user. * */
  public float getRotation() {
    return this.userRotation;
  }

  /** Sets pixel-length of new scalling (objective scaling, does not increment). * */
  public void setScaling(float scaling) {
    this.userScaling = scaling;
  }

  public float getScaling() {
    return this.userScaling;
  }

  /**
   * This method converts an ArrayList of stickers to a data string that can be passed to the
   * Mediapipe graph.
   *
   * @param stickerList ArrayList of Sticker objects to convert to data string
   * @return String of all sticker data (Managed by Android end)
   */
  public static String stickerArrayListToRawData(ArrayList<Sticker> stickerList) {
    String output = "";
    for (Sticker sticker : stickerList) {
      output +=
          ("\n(sticker_id:"
              + sticker.getStickerID()
              + ",\nsticker_anchor_x:"
              + sticker.getAnchor()[0]
              + ",\nsticker_anchor_y:"
              + sticker.getAnchor()[1]
              + ",\nsticker_rotation:"
              + sticker.getRotation()
              + ",\nsticker_scaling:"
              + sticker.getScaling()
              + ",\nsticker_render_id:"
              + sticker.renderID
              + ")");
    }
    return output;
  }
}
