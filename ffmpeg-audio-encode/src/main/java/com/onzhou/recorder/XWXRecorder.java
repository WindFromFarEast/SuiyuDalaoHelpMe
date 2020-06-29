package com.onzhou.recorder;

import android.os.Build;
import android.support.annotation.Keep;
import android.support.annotation.RequiresApi;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

@RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
@Keep
public class XWXRecorder implements SurfaceHolder.Callback {

    private static final String TAG = XWXResult.class.getSimpleName();

    private Surface mSurface;
    private SurfaceView mSurfaceView;
    private int mSurfaceWidth;
    private int mSurfaceHeight;

    private XWXCamera mCamera;
    private XWXCameraSetting mCamSetting;
    private TextureHolder mTextureHolder = new TextureHolder();

    private long mHandler;

    public XWXRecorder(SurfaceView surfaceView, XWXCameraSetting setting) {
        surfaceView.getHolder().addCallback(this);
        mSurfaceView = surfaceView;
        mHandler = nativeCreate();
        mCamera = new XWXCamera();
        mCamSetting = setting;
    }

    public int startPreview() {
        int ret = mCamera.open(true);
        if (ret != XWXResult.OK) {
            return ret;
        }
        mCamera.config(mCamSetting);
        ret = nativeStartPreview(mHandler, mSurface);
        if (ret != XWXResult.OK) {
            return ret;
        }
        return ret;
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        mSurface = holder.getSurface();
        mSurfaceWidth = mSurfaceView.getWidth();
        mSurfaceHeight = mSurfaceView.getHeight();

        //在底层创建GL线程用于显示Camera数据
        startPreview();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    //---------------------------------------------Use for native or native callback------------------------------------------------------------//

    private Callback.OnOpenGLCallback mOpenGLCallback = new Callback.OnOpenGLCallback() {
        @Override
        public int onGLCreated() {
            mTextureHolder.onCreate();
            mTextureHolder.setOnFrameAvailableListener(new OnFrameAvailableListener() {
                @Override
                public void onFrameAvailable(int texID, float[] mvp) {
                    updateRenderContent(mHandler, texID, mvp);
                }
            });
            mCamera.startPreview(mTextureHolder.getSurfaceTexture());

            return mTextureHolder.getSurfaceTextureId();
        }

        @Override
        public void onGLRunning() {
            mTextureHolder.updateTexImage();
        }

        @Override
        public void onGLDestroyed() {

        }
    };

    public int onGLCreated() {
        return mOpenGLCallback.onGLCreated();
    }

    public void onGLRunning() {
        mOpenGLCallback.onGLRunning();
    }

    public void onGLDestroyed() {
        mOpenGLCallback.onGLDestroyed();
    }

    private native long nativeCreate();
    private native int nativeStartPreview(long handler, Surface surface);
    private native int updateRenderContent(long handler, int texID, float[] mvp);
}
