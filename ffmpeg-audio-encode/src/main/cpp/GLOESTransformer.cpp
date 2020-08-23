//
// Created by xiangweixin on 2020-06-29.
//

#include "GLOESTransformer.h"

static const char *vertexShaderCode = {
        "attribute vec2 a_position;\n"
        "attribute vec2 a_texPosition;\n"
        "varying vec2 v_texPosition;\n"
        "void main() {\n"
        "gl_Position = vec4(a_position, 0.0, 1.0);\n"
        "v_texPosition = a_texPosition;\n"
        "}"
};
static const char *fragmentShaderCode = {
        "#extension GL_OES_EGL_image_external : require\n"
        "varying vec2 v_texPosition;\n"
        "uniform samplerExternalOES inputImageTexture;\n"
        "void main() {\n"
        "gl_FragColor = texture2D(inputImageTexture, v_texPosition);\n"
        "}"
};

int GLOESTransformer::init() {
    glGenTextures(1, &m_iFboTexID);
    if (m_iFboTexID <= 0) {
        LOGE("glGenTextures failed!");
        return GL_GEN_TEXTURE_ERROR;
    }
    glBindTexture(GL_TEXTURE_2D, m_iFboTexID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1280, 720, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &m_iFbo);
    if (m_iFbo <= 0) {
        LOGE("glGenFramebuffers failed!");
        return GL_GEN_FBO_ERROR;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, m_iFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_iFboTexID, 0);
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        LOGE("glGetError.. error is %d  %s:%d", error, __FUNCTION__, __LINE__);
        return error;
    }
    error = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (error != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("glCheckFramebufferStatus.. error is %d  %s:%d", error, __FUNCTION__, __LINE__);
        return error;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    int ret = m_program.init(vertexShaderCode, fragmentShaderCode);
    if (ret != 0) {
        LOGE("program init failed. ret is %d", ret);
        return ret;
    }
    m_program.use();
    m_iVertexPosLoc = glGetAttribLocation(m_program.getProgramID(), "a_position");
    if (m_iVertexPosLoc < 0) {
        LOGE("glGetAttribLocation error.");
        return GL_GET_ATTRIBUTE_LOC_ERROR;
    }
    m_iTexturePosLoc = glGetAttribLocation(m_program.getProgramID(), "a_texPosition");
    if (m_iTexturePosLoc < 0) {
        LOGE("glGetAttribLocation error.");
        return GL_GET_ATTRIBUTE_LOC_ERROR;
    }
    m_iTextureLoc = glGetUniformLocation(m_program.getProgramID(), "inputImageTexture");
    if (m_iTextureLoc < 0) {
        LOGE("glGetUniformLocation error.");
        return GL_GET_UNIFORM_LOC_ERROR;
    }
    m_program.unuse();

    return READY;
}

void GLOESTransformer::setTextureData(GLuint texID, Size texSize)  {
    m_iTexID = texID;
    m_sTexSize = texSize;
}

GLuint GLOESTransformer::transform() {
    m_program.use();

    glViewport(0, 0, m_sTexSize.width, m_sTexSize.height);
    glBindFramebuffer(GL_FRAMEBUFFER, m_iFbo);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, m_iTexID);

    float vertexData[] = {
            -1.f, 1.f,
            1.f, 1.f,
            -1.f, -1.f,
            1.f, -1.f
    };
    glEnableVertexAttribArray(m_iVertexPosLoc);
    glVertexAttribPointer(m_iVertexPosLoc, 2, GL_FLOAT, GL_FALSE, 0, vertexData);

    float textureData[] = {
            0.f, 1.f,
            1.f, 1.f,
            0.f, 0.f,
            1.f, 0.f
    };
    glEnableVertexAttribArray(m_iTexturePosLoc);
    glVertexAttribPointer(m_iTexturePosLoc, 2, GL_FLOAT, GL_FALSE, 0, textureData);

    glUniform1i(m_iTextureLoc, 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glFinish();

    glDisableVertexAttribArray(m_iVertexPosLoc);
    glDisableVertexAttribArray(m_iTexturePosLoc);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    m_program.unuse();

    return m_iFboTexID;
}