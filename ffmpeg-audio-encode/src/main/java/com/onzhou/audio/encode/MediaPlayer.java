package com.onzhou.audio.encode;

import android.view.Surface;

public class MediaPlayer {

    private long mHandler;

    public MediaPlayer() {
        mHandler = nativeCreate();
    }

    public int init(String path, Surface surface) {
        return nativeInit(mHandler, path, surface);
    }

    public int play() {
        return nativePlay(mHandler);
    }

    public void pause() {

    }

    public void stop() {

    }

    public int seek(int timestamp) {
        return 0;
    }

    private native long nativeCreate();

    private native int nativeInit(long handle, String path, Surface surface);

    private native int nativePlay(long handle);

}
