//
// Created by xiangweixin on 2020-06-29.
//

#ifndef FFMPEG4ANDROID_MASTER_GLTEXTUREUPLOADER_H
#define FFMPEG4ANDROID_MASTER_GLTEXTUREUPLOADER_H

#include "GLData.h"

/**
 * 用于将OES纹理转普通纹理
 */
class GLOESTransformer {

public:
    int init();
    void setTextureData(GLuint texID, Size texSize);
    GLuint transform();

private:
    GLuint m_iTexID = 0;
    Size m_sTexSize;
    GLuint m_iFbo = 0;
    GLuint m_iFboTexID = 0;

    GLint m_iVertexPosLoc = -1;
    GLint m_iTexturePosLoc = -1;
    GLint m_iTextureLoc = -1;

    GLProgram m_program;

};


#endif //FFMPEG4ANDROID_MASTER_GLTEXTUREUPLOADER_H
