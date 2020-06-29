//
// Created by xiangweixin on 2020/6/28.
//

#ifndef FFMPEG4ANDROID_MASTER_GLPROGRAM_H
#define FFMPEG4ANDROID_MASTER_GLPROGRAM_H

#include "GLES2/gl2.h"
#include "GLES2/gl2platform.h"
#include "logger.h"
#include "XWXResult.h"

class GLProgram {

public:
    int init(const char *vertexCode, const char *fragmentCode);
    void release();
    void use();
    void unuse();
    GLuint getProgramID();

private:
    GLuint m_iVertexShader = 0;
    GLuint m_iFragmentShader = 0;
    GLuint m_iProgram = 0;

};


#endif //FFMPEG4ANDROID_MASTER_GLPROGRAM_H
