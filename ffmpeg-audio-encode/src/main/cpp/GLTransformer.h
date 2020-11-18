//
// Created by xiangweixin on 2020/7/14.
//

#ifndef FFMPEG4ANDROID_MASTER_GLTRANSFORMER_H
#define FFMPEG4ANDROID_MASTER_GLTRANSFORMER_H

#include "GLData.h"

/**
 * setInputTexData->setOutputTexData->setRotate->setXXX->transform->getOutputTexture
 */
class GLTransformer {
public:
    int init();
    GLuint transform(GLuint inputTex, int width, int height);
    void setRotate(int degree) {
        m_iRotateDegree = degree;
    };
    void setFlip(bool flipX, bool flipY) {
        m_bFlipX = flipX;
        m_bFlipY = flipY;
    };

private:
    GLuint m_iFbo = 0;

    GLint m_iVertexPosLoc = -1;
    GLint m_iTexturePosLoc = -1;
    GLint m_iTextureLoc = -1;
    GLint m_iMvpLoc = -1;

    GLProgram m_program;

    int m_iRotateDegree = 0;
    bool m_bFlipX = false;
    bool m_bFlipY = false;
};


#endif //FFMPEG4ANDROID_MASTER_GLTRANSFORMER_H
