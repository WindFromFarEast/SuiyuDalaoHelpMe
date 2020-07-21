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
    GLuint transform(GLFrame &output);
    void setInputTexData(GLFrame frame) {
        m_iTexID = frame.texID;
        m_inSize = frame.texSize;
    };
    void setOutputTexData(Size outputSize) {
        m_outSize = outputSize;
    };
    void setRotate(int degree) {
        m_iRotateDegree = degree;
    };

private:
    GLuint m_iTexID = 0;

    Size m_inSize;
    Size m_outSize;

    GLuint m_iFbo = 0;
    GLuint m_iFboTexID = 0;

    GLint m_iVertexPosLoc = -1;
    GLint m_iTexturePosLoc = -1;
    GLint m_iTextureLoc = -1;
    GLint m_iMvpLoc = -1;

    GLProgram m_program;

    int m_iRotateDegree = 0;
};


#endif //FFMPEG4ANDROID_MASTER_GLTRANSFORMER_H
