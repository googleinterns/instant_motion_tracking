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
import android.net.Uri;
import android.os.Bundle;
import android.util.AttributeSet;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;

import androidx.appcompat.widget.AppCompatEditText;
import androidx.core.view.inputmethod.EditorInfoCompat;
import androidx.core.view.inputmethod.InputConnectionCompat;
import androidx.core.view.inputmethod.InputContentInfoCompat;

public class GIFEditText extends AppCompatEditText {

    private OnGIFCommit onGIFCommit;

    public GIFEditText(Context context) {
        super(context);
    }

    public GIFEditText(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public interface OnGIFCommit {
        void OnGIFCommit(Uri contentUri, ClipDescription description);
    }

    public void setGIFListener(OnGIFCommit onGIFCommit) {
        this.onGIFCommit = onGIFCommit;
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo editorInfo) {
        final InputConnection inputConnection = super.onCreateInputConnection(editorInfo);
        EditorInfoCompat.setContentMimeTypes(editorInfo, new String[]{"image/gif"});
        return InputConnectionCompat.createWrapper(inputConnection, editorInfo,
                new InputConnectionCompat.OnCommitContentListener() {
                    @Override
                    public boolean onCommitContent(final InputContentInfoCompat inputContentInfo,
                                                   int flags, Bundle opts) {
                            try {
                                if (onGIFCommit != null) {
                                    Runnable runnable = new Runnable() {
                                        @Override
                                        public void run() {
                                            inputContentInfo.requestPermission();
                                            onGIFCommit.OnGIFCommit(
                                                    inputContentInfo.getContentUri(),
                                                    inputContentInfo.getDescription());
                                            inputContentInfo.releasePermission();
                                        }
                                    };
                                    new Thread(runnable).start();
                                }
                            } catch (Exception e) {
                                e.printStackTrace();
                                return false;
                            }
                        return true;
                    }
                });
    }
}
