package com.onzhou.ui;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.view.SurfaceView;

import com.onzhou.audio.encode.R;
import com.onzhou.recorder.XWXCameraSetting;
import com.onzhou.recorder.XWXRecorder;

public class RecorderActivity extends AppCompatActivity {

    static {
        System.loadLibrary("native-encode");
    }

    private SurfaceView mSurfaceView;
    private XWXRecorder mRecorder;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_recorder);
        initView();
        XWXCameraSetting.Builder builder = new XWXCameraSetting.Builder();
        XWXCameraSetting setting = builder.setPreviewSize(1280, 720)
                .setPictureSize(1280, 720)
                .setFocusMode(XWXCameraSetting.FocusMode.AUTO)
                .build();
        mRecorder = new XWXRecorder(mSurfaceView, setting);
//        mRecorder.startPreview();
    }

    private void initView() {
        mSurfaceView = (SurfaceView) findViewById(R.id.surfaceView);
    }

}
