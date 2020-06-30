//
// Created by xiangweixin on 2020-06-28.
//

#ifndef FFMPEG4ANDROID_MASTER_GLDISPLAYER_H
#define FFMPEG4ANDROID_MASTER_GLDISPLAYER_H

#include "GLProgram.h"
#include "GLData.h"
#include "Matrix.h"

class GLDisplayer {
public:
    void setSurfaceSize(int width, int height);
    int init();
    void draw(GLFrame &frame);

private:
    GLProgram m_program;
    GLint m_positionLoc = -1;
    GLint m_texPositionLoc = -1;
    GLint m_mvpLoc = -1;
    GLint m_texLoc = -1;

    Size m_surfaceSize;
};


#endif //FFMPEG4ANDROID_MASTER_GLDISPLAYER_H
