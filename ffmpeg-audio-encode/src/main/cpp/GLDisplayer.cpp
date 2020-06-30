//
// Created by xiangweixin on 2020-06-28.
//

#include "GLDisplayer.h"

static const char *vertexShaderCode = {
        "precision mediump float;\n"
        "attribute vec2 a_position;\n"
        "attribute vec2 a_texPosition;\n"
        "varying vec2 v_texPosition;\n"
        "uniform mat4 mvpMatrix;\n"
        "void main() {\n"
        "gl_Position = vec4(a_position, 0.0, 1.0);\n"
        "v_texPosition = a_texPosition;\n"
        "}"
};
static const char *fragmentShaderCode = {
        "varying vec2 v_texPosition;\n"
        "uniform sampler2D inputImageTexture;\n"
        "void main() {\n"
        "gl_FragColor = texture2D(inputImageTexture, v_texPosition);\n"
        "}"
};

int GLDisplayer::init() {
    int ret = m_program.init(vertexShaderCode, fragmentShaderCode);
    if (ret != 0) {
        LOGE("GLDisplayer >>> init program failed. ret is %d", ret);
        return ret;
    }
    m_program.use();
    m_positionLoc = glGetAttribLocation(m_program.getProgramID(), "a_position");
    m_texPositionLoc = glGetAttribLocation(m_program.getProgramID(), "a_texPosition");
    m_texLoc = glGetUniformLocation(m_program.getProgramID(), "inputImageTexture");
    m_mvpLoc = glGetUniformLocation(m_program.getProgramID(), "mvpMatrix");
    m_program.unuse();
    return READY;
}

void GLDisplayer::draw(GLFrame &frame) {
    m_program.use();

    Size inputSize = frame.texSize;
    Size outputSize = m_surfaceSize;

    float inputRatio = inputSize.width * 1.0f / inputSize.height;
    float inputWidth = inputRatio;
    float inputHeight = 1.0f;
    Rect vertexRect = { -inputWidth / 2, inputWidth / 2, inputHeight / 2, -inputHeight / 2 };

    float outputRatio = outputSize.width * 1.0f / outputSize.height;
    float outputWidth = outputRatio;
    float outputHeight = 1.0f;

    float scale = fmaxf(outputWidth / inputWidth, outputHeight / inputHeight);

    auto mvpMatrix = new float[MATRIX_LENGTH];
    matrixSetIdentityM(mvpMatrix);

    auto orthoMatrix = new float[MATRIX_LENGTH];
    matrixSetIdentityM(orthoMatrix);
    ortho(orthoMatrix, -outputWidth / 2, outputWidth / 2, -outputHeight / 2, outputHeight / 2, -1, 1);

    auto scaleMatrix = new float[MATRIX_LENGTH];
    matrixScaleM(scaleMatrix, scale, scale, 0.f);

    matrixMultiplyMM(mvpMatrix, scaleMatrix, orthoMatrix);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, outputSize.width, outputSize.height);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, frame.texID);
    glEnableVertexAttribArray(m_positionLoc);
    glEnableVertexAttribArray(m_texPositionLoc);
    float vertexData[] = {
            vertexRect.left, vertexRect.top,
            vertexRect.right, vertexRect.top,
            vertexRect.left, vertexRect.bottom,
            vertexRect.right, vertexRect.bottom
    };
    glVertexAttribPointer(m_positionLoc, 2, GL_FLOAT, GL_FALSE, 0, vertexData);
    float textureData[] = {
            0.f, 1.f,
            1.f, 1.f,
            0.f, 0.f,
            1.f, 0.f
    };
    glVertexAttribPointer(m_texPositionLoc, 2, GL_FLOAT, GL_FALSE, 0, textureData);
    glUniform1i(m_texLoc, 0);

    glUniformMatrix4fv(m_mvpLoc, 1, GL_FALSE, mvpMatrix);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    GLenum gLenum = glGetError();
    if (gLenum != GL_NO_ERROR) {
        LOGE("glError.. error: %d", gLenum);
        return;
    }

    glFinish();

    glDisableVertexAttribArray(m_positionLoc);
    glDisableVertexAttribArray(m_texPositionLoc);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_program.unuse();

    delete[] mvpMatrix;
    delete[] orthoMatrix;
}

void GLDisplayer::setSurfaceSize(int width, int height) {
    m_surfaceSize = { width, height };
}
