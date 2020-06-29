package com.onzhou.recorder;

import android.graphics.SurfaceTexture;
import android.opengl.EGL14;
import android.opengl.EGLContext;
import android.opengl.GLES20;

public class TextureHolder {

    private int mSurfaceTextureId;
    private SurfaceTexture mSurfaceTexture;
    private OnFrameAvailableListener mListener;

    public void onCreate() {
        mSurfaceTextureId = TextureManager.getManager().createTexture(true);

        mSurfaceTexture = new SurfaceTexture(mSurfaceTextureId);
        mSurfaceTexture.setOnFrameAvailableListener(new SurfaceTexture.OnFrameAvailableListener() {
            @Override
            public void onFrameAvailable(SurfaceTexture surfaceTexture) {
                LogUtil.d("TextureHolder", "onFrameAvailable...");
                if (mListener != null) {
                    float[] matrix = new float[16];
                    surfaceTexture.getTransformMatrix(matrix);
                    mListener.onFrameAvailable(mSurfaceTextureId, matrix);
                }
            }
        });
        mSurfaceTexture.setDefaultBufferSize(1280, 720);
    }

    public void updateTexImage() {
        LogUtil.d("TextureHolder", "updateTexImage...");
        if (mSurfaceTexture == null) {
            LogUtil.d("TextureHolder", "SurfaceTexture is null, ignore updateTexImage");
            return;
        }
        EGLContext context = EGL14.eglGetCurrentContext();
        boolean isGLThread = context != EGL14.EGL_NO_CONTEXT;
        if (!isGLThread) {
            LogUtil.e("TextureHolder", "call updateTexImage in non-gl thread!");
            return;
        }
        mSurfaceTexture.updateTexImage();
    }

    public SurfaceTexture getSurfaceTexture() {
        return mSurfaceTexture;
    }

    public int getSurfaceTextureId() {
        return mSurfaceTextureId;
    }

    public void setOnFrameAvailableListener(OnFrameAvailableListener onFrameAvailableListener) {
        this.mListener = onFrameAvailableListener;
    }

}
