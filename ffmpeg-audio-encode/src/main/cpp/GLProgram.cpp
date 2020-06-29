//
// Created by xiangweixin on 2020/6/28.
//

#include "GLProgram.h"

int GLProgram::init(const char *vertexCode, const char *fragmentCode) {
    m_iVertexShader = glCreateShader(GL_VERTEX_SHADER);
    if (m_iVertexShader > 0) {
        glShaderSource(m_iVertexShader, 1, &vertexCode, nullptr);
        glCompileShader(m_iVertexShader);
        GLint status = GL_FALSE;
        glGetShaderiv(m_iVertexShader, GL_COMPILE_STATUS, &status);
        if (status != GL_TRUE) {
            GLchar log[1024];
            GLsizei length;
            glGetShaderInfoLog(m_iVertexShader, 1024, &length, log);
            LOGE("glCompile vertex shader failed. ret is %d, info is %s", status, log);
            release();
            return GL_COMPILE_ERROR;
        }
    }

    m_iFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    if (m_iFragmentShader > 0) {
        glShaderSource(m_iFragmentShader, 1, &fragmentCode, nullptr);
        glCompileShader(m_iFragmentShader);
        GLint status = GL_FALSE;
        glGetShaderiv(m_iFragmentShader, GL_COMPILE_STATUS, &status);
        if (status != GL_TRUE) {
            GLchar log[1024];
            GLsizei length;
            glGetShaderInfoLog(m_iFragmentShader, 1024, &length, log);
            LOGE("glCompile fragment shader failed. ret is %d, info is %s", status, log);
            release();
            return GL_COMPILE_ERROR;
        }
    }

    m_iProgram = glCreateProgram();
    if (m_iProgram == 0) {
        LOGE("glCreateProgram failed. ret is %d", m_iProgram);
        release();
        return GL_CREATE_PROGRAM_ERROR;
    }

    glAttachShader(m_iProgram, m_iVertexShader);
    glAttachShader(m_iProgram, m_iFragmentShader);
    glLinkProgram(m_iProgram);
    GLint linkStatus = GL_FALSE;
    glGetProgramiv(m_iProgram, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE) {
        GLchar log[1024];
        GLsizei length;
        glGetProgramInfoLog(m_iProgram, 1024, &length, log);
        LOGE("glLinkProram failed. ret is %d, log: %s", linkStatus, log);
        release();
        return GL_LINK_PROGRAM_ERROR;
    }

    return READY;

}

void GLProgram::use() {
    if (m_iProgram != 0) {
        glUseProgram(m_iProgram);
    }
}

void GLProgram::unuse() {
    glUseProgram(0);
}

GLuint GLProgram::getProgramID() {
    return m_iProgram;
}

void GLProgram::release() {
    if (m_iVertexShader != 0) {
        glDeleteShader(m_iVertexShader);
        m_iVertexShader = 0;
    }
    if (m_iFragmentShader != 0) {
        glDeleteShader(m_iFragmentShader);
        m_iFragmentShader = 0;
    }
    if (m_iProgram != 0) {
        glDeleteProgram(m_iProgram);
        m_iProgram = 0;
    }
}