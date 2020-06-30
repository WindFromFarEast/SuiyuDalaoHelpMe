//
// Created by xiangweixin on 2020-02-29.
//

#ifndef MYOWNSTUDY_RECORDEROPENGLPROXY_H
#define MYOWNSTUDY_RECORDEROPENGLPROXY_H

#include <sys/types.h>
#include <pthread.h>
#include <jni.h>
#include <string>
#include "android/log.h"
#include "functional"
#include <atomic>
#include "XWXResult.h"
#include "GLData.h"
#include "GLOESTransformer.h"
#include "GLDisplayer.h"

void *render_thread(void *args);

typedef struct RecordEnv {
    jobject mObj;
} RecorderEnv;

class RecorderOpenGLProxy {
    friend void *render_thread(void *args);
public:
    RecorderOpenGLProxy();
    ~RecorderOpenGLProxy();

    int startRender(ANativeWindow *window);
    int swapBuffers();
    int initRenderEnv();

    void stopRender();

    int initEGL();
    void destroyEGL();
    void updateRenderContent(GLuint surfaceTextureID, float mvp[]);

    void display(GLuint texID);

    void setOnOpenGLCreateCallback(std::function<int(void*)> func) { m_onOpenGLCreateCallback = func; };
    void setOnOpenGLRunningCallback(std::function<void(void*)> func) { m_onOpenGLRunningCallback = func; };
    void setOnOpenGLDestroyCallback(std::function<void(void*)> func) { m_onOpenGLDestroyCallback = func; };

    void setSurfaceTextureID(GLuint id) { m_iSurfaceTextureID = id; };

public:
    GLuint m_iSurfaceTextureID = 0;
    float *m_pTexMatrix = nullptr;
    RecorderEnv recorderEnv;

private:
    std::function<int(void*)> m_onOpenGLCreateCallback;
    std::function<void(void*)> m_onOpenGLDestroyCallback;
    std::function<void(void*)> m_onOpenGLRunningCallback;

private:
    pthread_t m_threadId;//GL线程tid
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
    ANativeWindow *m_window = nullptr;
    EGLDisplay m_display = EGL_NO_DISPLAY;
    EGLContext m_context = EGL_NO_CONTEXT;
    EGLSurface m_surface = EGL_NO_SURFACE;

    int m_iSurfaceWidth = 0;
    int m_iSurfaceHeight = 0;

    std::atomic_bool m_bStopRender;

    GLDisplayer m_glDisplayer;
    GLOESTransformer m_glOESTransformer;
};


#endif //MYOWNSTUDY_RECORDEROPENGLPROXY_H
