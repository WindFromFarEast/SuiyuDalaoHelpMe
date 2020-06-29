package com.onzhou.recorder;

public interface Callback {

    interface OnOpenGLCallback {
        int onGLCreated();
        void onGLRunning();
        void onGLDestroyed();
    }

}
