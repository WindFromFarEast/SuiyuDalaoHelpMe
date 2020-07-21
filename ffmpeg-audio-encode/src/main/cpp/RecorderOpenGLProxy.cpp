//
// Created by xiangweixin on 2020-02-29.
//
#include "RecorderOpenGLProxy.h"
#include <jni.h>
#include "GLUtils.h"

#define TAG "RecorderOpenGLProxy"

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
    int ret = m_glDisplayer.init();
    m_glDisplayer.setSurfaceSize(m_iSurfaceWidth, m_iSurfaceHeight);
    if (ret != 0) {
        LOGE("glDisplayer init failed. ret is %d", ret);
        return ret;
    }
    ret = m_glOESTransformer.init();
    if (ret != 0) {
        LOGE("glOESTransformer init failed. ret is %d", ret);
        return ret;
    }
    ret = m_glTransformer.init();
    if (ret != 0) {
        LOGE("glTransformer init failed. ret is %d", ret);
        return ret;
    }
    return READY;
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

void RecorderOpenGLProxy::display(GLuint texID) {
    GLFrame frame = {
            .texID = texID,
            .texSize = { 720, 1280 },
    };
    m_glDisplayer.draw(frame);
}

static int counts = 0;
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

    Size outSize = {
            .width = 720,
            .height = 1280
    };
    proxy->m_glTransformer.setOutputTexData(outSize);
    proxy->m_glTransformer.setRotate(90);
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

        Size oesTexSize = {
                .width = 1280,
                .height = 720
        };
        proxy->m_glOESTransformer.setTextureData(proxy->m_iSurfaceTextureID, oesTexSize);
        GLuint commonTexID = proxy->m_glOESTransformer.transform();
//        char name[1024];
//        sprintf(name, "sdcard/rgba/%d.rgba", counts++);
//        writeTextureToFile(commonTexID, 1280, 720, name);

        GLFrame afterOesFrame = {
                .texID = commonTexID,
                .texSize = {.width = 1280, .height = 720},
        };
        proxy->m_glTransformer.setInputTexData(afterOesFrame);
        GLFrame transformFrame;
        proxy->m_glTransformer.transform(transformFrame);

        proxy->display(transformFrame.texID);
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