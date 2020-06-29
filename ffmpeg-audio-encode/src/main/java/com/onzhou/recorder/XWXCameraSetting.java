package com.onzhou.recorder;

import android.os.Build;
import android.support.annotation.RequiresApi;
import android.util.Size;

@RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
public class XWXCameraSetting {

    public enum FocusMode {
        AUTO, FIXED, EDOF, INFINITY, MACRO
    }

    private Size previewSize;
    private Size pictureSize;

    private FocusMode focusMode;

    //don't allow to new this class directly. Please use builder to build a setting object.
    private XWXCameraSetting() { }

    public Size getPreviewSize() {
        return previewSize;
    }

    public Size getPictureSize() {
        return pictureSize;
    }

    public FocusMode getFocusMode() {
        return focusMode;
    }

    public static class Builder {

        private XWXCameraSetting mCamSetting;

        public Builder() {
            mCamSetting = new XWXCameraSetting();
        }

        public Builder setPreviewSize(int width, int height) {
            Size size = new Size(width, height);
            mCamSetting.previewSize = size;
            return this;
        }

        public Builder setPictureSize(int width, int height) {
            Size size = new Size(width, height);
            mCamSetting.pictureSize = size;
            return this;
        }

        public Builder setFocusMode(FocusMode mode) {
            mCamSetting.focusMode = mode;
            return this;
        }

        public XWXCameraSetting build() {
            return mCamSetting;
        }

    }

}
