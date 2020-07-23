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
import android.util.Log;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;

import androidx.appcompat.widget.AppCompatEditText;
import androidx.core.view.inputmethod.EditorInfoCompat;
import androidx.core.view.inputmethod.InputConnectionCompat;
import androidx.core.view.inputmethod.InputContentInfoCompat;

import static android.os.Build.VERSION.SDK_INT;
import static android.os.Build.VERSION_CODES.N_MR1;

public class GIFEditText extends AppCompatEditText {
    public GIFEditText(Context context) {
        super(context);
    }

    public GIFEditText(Context context, AttributeSet attrs) {
        super(context, attrs);
        setAttributes(context, attrs);
    }

    public GIFEditText(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        setAttributes(context, attrs);
    }

    private void setAttributes(Context context, AttributeSet attrs) {
        this.mimeTypes = (new String[]{"image/gif"});
    }

    public interface OnRichContentListener {

        void onRichContent(Uri contentUri, ClipDescription description);
    }

    private OnRichContentListener onRichContentListener = null;
    private String[] mimeTypes = {};
    private static final String TAG = "GIFEditText";
    public boolean runListenerInBackground = true;

    public void setOnRichContentListener(OnRichContentListener onRichContentListener) {
        this.onRichContentListener = onRichContentListener;
    }

    public String[] getContentMimeTypes() {
        return mimeTypes;
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo editorInfo) {
        final InputConnection ic = super.onCreateInputConnection(editorInfo);
        EditorInfoCompat.setContentMimeTypes(editorInfo, getContentMimeTypes());
        return InputConnectionCompat.createWrapper(ic, editorInfo,
                new InputConnectionCompat.OnCommitContentListener() {
                    @Override
                    public boolean onCommitContent(final InputContentInfoCompat inputContentInfo,
                                                   int flags, Bundle opts) {
                        if (SDK_INT >= N_MR1 && (flags & InputConnectionCompat
                                .INPUT_CONTENT_GRANT_READ_URI_PERMISSION) != 0) {
                            try {
                                if (onRichContentListener != null) {
                                    Runnable runnable = new Runnable() {
                                        @Override
                                        public void run() {
                                            inputContentInfo.requestPermission();
                                            onRichContentListener.onRichContent(
                                                    inputContentInfo.getContentUri(),
                                                    inputContentInfo.getDescription());
                                            inputContentInfo.releasePermission();
                                        }
                                    };
                                    if (runListenerInBackground) new Thread(runnable).start();
                                    else runnable.run();
                                }
                            } catch (Exception e) {
                                Log.e(TAG, "Error accepting rich content: " + e.getMessage());
                                e.printStackTrace();
                                return false;
                            }
                        }
                        return true;
                    }
                });
    }
}
