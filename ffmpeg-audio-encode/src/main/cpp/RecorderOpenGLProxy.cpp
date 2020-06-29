//
// Created by xiangweixin on 2020-02-29.
//
#include "RecorderOpenGLProxy.h"
#include <jni.h>
#include "GLES2/gl2ext.h"
#include "Matrix.h"
#include "ShaderCode.h"
#include "GLData.h"

#define TAG "RecorderOpenGLProxy"

static const char *vertexShaderCode = {
        "precision mediump float;\n"
        "attribute vec4 a_position;\n"
        "attribute vec2 a_texturePosition;\n"
        "varying vec2 v_texturePosition;\n"
        "uniform mat4 mvpMatrix;\n"
        "uniform mat4 textureMatrix;\n"
        "void main() {\n"
        "v_texturePosition = (textureMatrix * vec4(a_texturePosition, 0.0, 1.0)).xy;\n"
        "gl_Position = mvpMatrix * a_position;\n"
        "}"
};

static const char *fragmentShaderCode = {
        "#extension GL_OES_EGL_image_external : require\n"
        "precision mediump float;\n"
        "varying vec2 v_texturePosition;\n"
        "uniform samplerExternalOES u_texture;\n"
        "void main() {\n"
        "gl_FragColor = texture2D(u_texture, v_texturePosition);\n"
        "}"
};

RecorderOpenGLProxy::RecorderOpenGLProxy() : m_bStopRender(false) {
    pthread_mutex_init(&m_mutex, nullptr);
    pthread_cond_init(&m_cond, nullptr);

    m_pTexMatrix = new float[MATRIX_LENGTH];
    matrixSetIdentityM(m_pTexMatrix);
}

RecorderOpenGLProxy::~RecorderOpenGLProxy() {
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_cond);
}

int RecorderOpenGLProxy::startRender(ANativeWindow *window) {
    m_window = window;
    m_iSurfaceWidth = ANativeWindow_getWidth(window);
    m_iSurfaceHeight = ANativeWindow_getHeight(window);

    return pthread_create(&m_threadId, nullptr, render_thread, this);
}

int RecorderOpenGLProxy::initEGL() {
    //↓ init egl environment.
    m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (m_display == EGL_NO_DISPLAY) {
        LOGE("init render env failed.");
        return EGL_GET_DISPLAY_ERROR;
    }

    auto version = new EGLint[2];
    if (!eglInitialize(m_display, &version[0], &version[1])) {
        LOGE("eglInitialize failed.");
        return EGL_INIT_ERROR;
    }

    const EGLint attrib_config_list[] = {
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_DEPTH_SIZE, 8,
            EGL_STENCIL_SIZE, 8,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_NONE
    };
    EGLint num_config;
    EGLConfig eglConfig;
    if (!eglChooseConfig(m_display, attrib_config_list, &eglConfig, 1, &num_config)) {
        LOGE("eglChooseConfig failed.");
        return EGL_CHOOSE_CONFIG_ERROR;
    }

    const EGLint attrib_ctx_list[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
    };
    m_context = eglCreateContext(m_display, eglConfig, nullptr, attrib_ctx_list);
    if (m_context == EGL_NO_CONTEXT) {
        LOGE("eglCreateContext failed.");
        return EGL_CREATE_CONTEXT_ERROR;
    }

    m_surface = eglCreateWindowSurface(m_display, eglConfig, m_window, nullptr);
    if (m_surface == EGL_NO_SURFACE) {
        LOGE("eglCreateWindowSurface failed.");
        return EGL_CREATE_WINDOWSURFACE_ERROR;
    }

    if (!eglMakeCurrent(m_display, m_surface, m_surface, m_context)) {
        LOGE("eglMakeCurrent failed.");
        return EGL_MAKE_CURRENT_ERROR;
    }

    return READY;
}

int RecorderOpenGLProxy::swapBuffers() {
    if (m_display != EGL_NO_DISPLAY && m_surface != EGL_NO_SURFACE && eglSwapBuffers(m_display, m_surface)) {
        return READY;
    }
    return EGL_SWAP_BUFFER_ERROR;
}

int RecorderOpenGLProxy::initRenderEnv() {
    int ret;
    m_DisplayProgram = new GLProgram;
    ret = m_DisplayProgram->init(vertexShaderCode, fragmentShaderCode);
    if (ret != 0) {
        LOGE("program init failed. ret is %d", ret);
        return ret;
    }
    m_DisplayProgram->use();
    m_iPositionLoc = glGetAttribLocation(m_DisplayProgram->getProgramID(), "a_position");
    m_iTexPositionLoc = glGetAttribLocation(m_DisplayProgram->getProgramID(), "a_texturePosition");
    m_iMVPLoc = glGetUniformLocation(m_DisplayProgram->getProgramID(), "mvpMatrix");
    m_iTextureLoc = glGetUniformLocation(m_DisplayProgram->getProgramID(), "u_texture");
    m_iTextureMatrixLoc = glGetUniformLocation(m_DisplayProgram->getProgramID(), "textureMatrix");
    m_DisplayProgram->unuse();
    return ret;
}

void RecorderOpenGLProxy::stopRender() {
    m_bStopRender = true;
}

void RecorderOpenGLProxy::destroyEGL() {
    if (m_display != EGL_NO_DISPLAY) {
        eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (m_surface != EGL_NO_SURFACE) {
            eglDestroySurface(m_display, m_surface);
            m_surface = EGL_NO_SURFACE;
        }
        if (m_context != EGL_NO_CONTEXT) {
            eglDestroyContext(m_display, m_context);
            m_context = EGL_NO_CONTEXT;
        }
        if (m_display != EGL_NO_DISPLAY) {
            eglTerminate(m_display);
            m_display = EGL_NO_DISPLAY;
        }
    }
}

void RecorderOpenGLProxy::updateRenderContent(GLuint surfaceTextureID, float *mvp) {
    pthread_mutex_lock(&m_mutex);
    m_iSurfaceTextureID = surfaceTextureID;
    memcpy(m_pTexMatrix, mvp, sizeof(float) * MATRIX_LENGTH);
    pthread_cond_signal(&m_cond);
    pthread_mutex_unlock(&m_mutex);
}

void RecorderOpenGLProxy::render() {
    m_DisplayProgram->use();

    Size inputSize = { 1280, 720 };
    Size outputSize = { m_iSurfaceWidth, m_iSurfaceHeight };

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
    glViewport(0, 0, m_iSurfaceWidth, m_iSurfaceHeight);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, m_iSurfaceTextureID);
    glEnableVertexAttribArray(m_iPositionLoc);
    glEnableVertexAttribArray(m_iTexPositionLoc);
    float vertexData[] = {
            vertexRect.left, vertexRect.top,
            vertexRect.right, vertexRect.top,
            vertexRect.left, vertexRect.bottom,
            vertexRect.right, vertexRect.bottom
    };
    glVertexAttribPointer(m_iPositionLoc, 2, GL_FLOAT, GL_FALSE, 0, vertexData);
    float textureData[] = {
            0.f, 1.f,
            1.f, 1.f,
            0.f, 0.f,
            1.f, 0.f
    };
    glVertexAttribPointer(m_iTexPositionLoc, 2, GL_FLOAT, GL_FALSE, 0, textureData);
    glUniformMatrix4fv(m_iTextureMatrixLoc, 1, GL_FALSE, m_pTexMatrix);
    glUniformMatrix4fv(m_iMVPLoc, 1, GL_FALSE, mvpMatrix);
    glUniform1i(m_iTextureLoc, 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glFinish();

    glDisableVertexAttribArray(m_iPositionLoc);
    glDisableVertexAttribArray(m_iTexPositionLoc);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_DisplayProgram->unuse();

    delete[] mvpMatrix;
    delete[] orthoMatrix;
}

void *render_thread(void *args) {
    auto proxy = reinterpret_cast<RecorderOpenGLProxy *>(args);
    if (!proxy) {
        LOGE("RecorderProxy is nullptr. %s:%d",__FUNCTION__, __LINE__);
        return nullptr;
    }

    int ret = proxy->initEGL();
    if (ret != 0) {
        LOGE("init egl failed. ret is %d", ret);
        return nullptr;
    }

    if (proxy->m_onOpenGLCreateCallback != nullptr) {
        proxy->m_onOpenGLCreateCallback(&proxy->recorderEnv);
    }

    ret = proxy->initRenderEnv();
    if (ret != 0) {
        LOGE("initRenderEnv failed. ret is %d", ret);
        return nullptr;
    }

    //render loop
    while (!proxy->m_bStopRender) {
        pthread_mutex_lock(&proxy->m_mutex);
        pthread_cond_wait(&proxy->m_cond, &proxy->m_mutex);
        if (proxy->m_onOpenGLRunningCallback != nullptr) {
            proxy->m_onOpenGLRunningCallback(&proxy->recorderEnv);
        }
        proxy->render();
        proxy->swapBuffers();
        pthread_mutex_unlock(&proxy->m_mutex);
    }

    if (proxy->m_onOpenGLDestroyCallback != nullptr) {
        proxy->m_onOpenGLDestroyCallback(&proxy->recorderEnv);
    }
    proxy->destroyEGL();

    pthread_detach(pthread_self());

    return nullptr;
}