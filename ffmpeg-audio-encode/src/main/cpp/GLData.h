//
// Created by xiangweixin on 2020-06-28.
//

#ifndef FFMPEG4ANDROID_MASTER_GLDATA_H
#define FFMPEG4ANDROID_MASTER_GLDATA_H

#include <GLES2/gl2.h>
#include <GLES2/gl2platform.h>
#include <GLES2/gl2ext.h>
#include "Matrix.h"
#include "ShaderCode.h"
#include "GLProgram.h"
#include "EGL/egl.h"
#include <android/native_window.h>

typedef struct Size {
    int width;
    int height;
} Size;

typedef struct Rect {
    float left;
    float right;
    float top;
    float bottom;
} Rect;

typedef struct GLFrame {
    GLuint texID;
    Size texSize;
} GLFrame;

#endif //FFMPEG4ANDROID_MASTER_GLDATA_H
