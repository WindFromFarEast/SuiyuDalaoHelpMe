package com.onzhou.recorder;

import android.opengl.GLES20;

public class GLProgram {

    private static final String TAG = GLProgram.class.getSimpleName();

    private int vertexShader = 0;
    private int fragmentShader = 0;
    private int program = 0;

    public int init(String vertexCode, String fragmentCode) {
        int vertexShader = GLES20.glCreateShader(GLES20.GL_VERTEX_SHADER);
        if (vertexShader > 0) {
            GLES20.glShaderSource(vertexShader, vertexCode);
            GLES20.glCompileShader(vertexShader);
            int[] compile = new int[1];
            GLES20.glGetShaderiv(vertexShader, GLES20.GL_COMPILE_STATUS, compile, 0);
            if (compile[0] != GLES20.GL_TRUE) {
                LogUtil.e(TAG, "Compile vertex shader failed.");
                GLES20.glDeleteShader(vertexShader);
                this.vertexShader = 0;
                return -1;
            }
            this.vertexShader = vertexShader;
        }

        int fragmentShader = GLES20.glCreateShader(GLES20.GL_FRAGMENT_SHADER);
        if (fragmentShader > 0) {
            GLES20.glShaderSource(fragmentShader, fragmentCode);
            GLES20.glCompileShader(fragmentShader);
            int[] compile = new int[1];
            GLES20.glGetShaderiv(fragmentShader, GLES20.GL_COMPILE_STATUS, compile, 0);
            if (compile[0] != GLES20.GL_TRUE) {
                LogUtil.e(TAG, "Compile fragment shader failed.");
                GLES20.glDeleteShader(fragmentShader);
                this.fragmentShader = 0;
                return -1;
            }
            this.fragmentShader = fragmentShader;
        }

        int program = GLES20.glCreateProgram();
        if (program == 0) {
            LogUtil.e(TAG, "Create program failed.");
            return -1;
        }
        GLES20.glAttachShader(program, this.vertexShader);
        GLES20.glAttachShader(program, this.fragmentShader);
        GLES20.glLinkProgram(program);
        int[] linkStatus = new int[1];
        GLES20.glGetProgramiv(program, GLES20.GL_LINK_STATUS, linkStatus, 0);
        if (linkStatus[0] != GLES20.GL_TRUE) {
            LogUtil.e(TAG, "Link program failed.");
            GLES20.glDeleteProgram(program);
            this.program = 0;
            return -1;
        }
        this.program = program;

        return 0;
    }

    public void use() {
        if (program > 0) {
            GLES20.glUseProgram(program);
        } else {
            LogUtil.e(TAG, "program has not been initialized.");
        }
    }

    public void unuse() {
        GLES20.glUseProgram(0);
    }

    public int getProgramID() {
        return program;
    }

    public void release() {
        if (vertexShader > 0) {
            GLES20.glDeleteShader(vertexShader);
        }
        if (fragmentShader > 0) {
            GLES20.glDeleteShader(fragmentShader);
        }
        if (program > 0) {
            GLES20.glDeleteProgram(program);
        }
    }

}