
#include "native_code.h"
#include "encode_aac.h"
#include "MediaPlayer.h"
#include "logger.h"
#include "RecorderOpenGLProxy.h"

/**
 * 动态注册
 */
JNINativeMethod methods[] = {
        {"onAudioFrame",     "([BI)V",                (void *) onAudioFrame},
        {"encodeAudioStart", "(Ljava/lang/String;)V", (void *) encodeAudioStart},
        {"encodeAudioStop",  "()V",                   (void *) encodeAudioStop}
};

/**
 * 动态注册
 * @param env
 * @return
 */
jint registerNativeMethod(JNIEnv *env) {
    jclass cl = env->FindClass("com/onzhou/audio/encode/NativeAudioEncoder");
    if ((env->RegisterNatives(cl, methods, sizeof(methods) / sizeof(methods[0]))) < 0) {
        return -1;
    }
    return 0;
}

/**
 * 加载默认回调
 * @param vm
 * @param reserved
 * @return
 */
//jint JNI_OnLoad(JavaVM *vm, void *reserved) {
//    JNIEnv *env = NULL;
//    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
//        return -1;
//    }
//    //注册方法
//    if (registerNativeMethod(env) != JNI_OK) {
//        return -1;
//    }
//
//
//
//    return JNI_VERSION_1_6;
//}


AACEncoder *pAACEncoder = NULL;

/**
 * 主要用来接收音频数据
 * @param env
 * @param obj
 * @param jpcmBuffer
 * @param length
 */
void onAudioFrame(JNIEnv *env, jobject obj, jbyteArray jpcmBuffer, jint length) {
    if (NULL != pAACEncoder) {
        jbyte *buffer = env->GetByteArrayElements(jpcmBuffer, NULL);
        pAACEncoder->EncodeBuffer((uint8_t *) buffer, length);
        env->ReleaseByteArrayElements(jpcmBuffer, buffer, NULL);
    }
}

/**
 * 开始录制音频初始化工作
 * @param env
 * @param obj
 * @param jpath
 */
void encodeAudioStart(JNIEnv *env, jobject obj, jstring jpath) {
    if (NULL == pAACEncoder) {
        pAACEncoder = new AACEncoder();
        const char *aacPath = env->GetStringUTFChars(jpath, NULL);
        pAACEncoder->EncodeStart(aacPath);
    }
}

/**
 * 结束音频录制收尾工作
 * @param env
 * @param obj
 */
void encodeAudioStop(JNIEnv *env, jobject obj) {
    if (NULL != pAACEncoder) {
        pAACEncoder->EncodeStop();
        delete pAACEncoder;
        pAACEncoder = NULL;
    }
}

#define GET_PLAYER() MediaPlayer *player = reinterpret_cast<MediaPlayer *>(handle);\
    if (player == nullptr) {\
        LOGE("player is null.");\
        return -1;\
    }

extern "C" JNIEXPORT jlong JNICALL
Java_com_onzhou_audio_encode_MediaPlayer_nativeCreate(JNIEnv *env, jobject obj) {
    MediaPlayer *player = new MediaPlayer;
    return reinterpret_cast<long>(player);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_onzhou_audio_encode_MediaPlayer_nativeInit(JNIEnv *env, jobject thiz, jlong handle,
                                                    jstring path, jobject surface) {
    GET_PLAYER();
    const char *path_ = env->GetStringUTFChars(path, nullptr);
    string pathStr(path_);
    ANativeWindow *window;
    window = ANativeWindow_fromSurface(env, surface);
    int ret = player->init(pathStr, window);
    env->ReleaseStringUTFChars(path, path_);
    return ret;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_onzhou_audio_encode_MediaPlayer_nativePlay(JNIEnv *env, jobject thiz, jlong handle) {
    GET_PLAYER();
    return player->play();
}

//-------------------------------------------Recorder--------------------------------------------

#define TAG_RECORDER "XWXRecorder"

static jmethodID onOpenGLCreateMethod = nullptr;
static jmethodID onOpenGLRunningMethod = nullptr;
static jmethodID onOpenGLDestroyedMethod = nullptr;
static JavaVM *mJavaVM;
static pthread_key_t mThreadKey;

static void Android_JNI_ThreadDestroyed(void *value);
static JNIEnv *Android_JNI_GetEnv(void);

static void Android_JNI_ThreadDestroyed(void *value) {
    JNIEnv *env = (JNIEnv *) value;
    if (env != NULL) {
        mJavaVM->DetachCurrentThread();
        pthread_setspecific(mThreadKey, NULL);
    }
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv *env;
    mJavaVM = vm;
    if (mJavaVM->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        LOGE("Failed to get the environment using GetEnv()");
        return -1;
    }

    if (pthread_key_create(&mThreadKey, Android_JNI_ThreadDestroyed) != JNI_OK) {
        LOGE("Error initializing pthread key");
    }

    return JNI_VERSION_1_4;
}

static JNIEnv *Android_JNI_GetEnv(void) {
    JNIEnv *env = nullptr;
    int status = mJavaVM->GetEnv((void **) &env, JNI_VERSION_1_4);
    if (status < 0) {
        status = mJavaVM->AttachCurrentThread(&env, NULL);
        if (status < 0) {
            LOGE("failed to attach current thread");
            return 0;
        }

        pthread_setspecific(mThreadKey, (void *) env);
    }
    return env;
}

static void loadMethods(JNIEnv *env) {
    jclass reocrderClass = env->FindClass("com/onzhou/recorder/XWXRecorder");
    if (reocrderClass == nullptr) {
        LOGE("find xwxrecorder class failed.");
        return;
    }
    onOpenGLCreateMethod = env->GetMethodID(reocrderClass, "onGLCreated", "()I");
    if (onOpenGLCreateMethod == nullptr) {
        LOGE("find onOpenGLCreateMethod failed.");
        return;
    }
    onOpenGLRunningMethod = env->GetMethodID(reocrderClass, "onGLRunning", "()V");
    if (onOpenGLRunningMethod == nullptr) {
        LOGE("find onOpenGLRunningMethod failed.");
        return;
    }
    onOpenGLDestroyedMethod = env->GetMethodID(reocrderClass, "onGLDestroyed", "()V");
    if (onOpenGLDestroyedMethod == nullptr) {
        LOGE("find onOpenGLDestroyedMethod failed.");
        return;
    }
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_onzhou_recorder_XWXRecorder_nativeCreate(JNIEnv *env, jobject thiz) {
    loadMethods(env);
    RecorderOpenGLProxy *recorderOpenGLProxy = new RecorderOpenGLProxy;

    recorderOpenGLProxy->recorderEnv.mObj = env->NewGlobalRef(thiz);
    recorderOpenGLProxy->setOnOpenGLCreateCallback([recorderOpenGLProxy](void *ptr) -> int {
        RecorderEnv *pEnv = static_cast<RecorderEnv*>(ptr);
        JNIEnv *env = Android_JNI_GetEnv();
        if (env != nullptr && onOpenGLCreateMethod != nullptr) {
            jint srcTexId = env->CallIntMethod(pEnv->mObj, onOpenGLCreateMethod);
            recorderOpenGLProxy->setSurfaceTextureID(static_cast<GLuint>(srcTexId));//在这里拿到Java层创建的SurfaceTexture纹理ID
        }
        return READY;
    });

    recorderOpenGLProxy->setOnOpenGLRunningCallback([] (void *ptr) {
        RecorderEnv *pEnv = static_cast<RecorderEnv*>(ptr);
        JNIEnv *env = Android_JNI_GetEnv();
        if (env != nullptr && onOpenGLRunningMethod != nullptr) {
            env->CallVoidMethod(pEnv->mObj, onOpenGLRunningMethod);
        }
    });

    recorderOpenGLProxy->setOnOpenGLDestroyCallback([](void *ptr) {
        RecorderEnv *pEnv = static_cast<RecorderEnv*>(ptr);
        JNIEnv *env = Android_JNI_GetEnv();
        if (env != nullptr && onOpenGLDestroyedMethod != nullptr) {
            env->CallVoidMethod(pEnv->mObj, onOpenGLDestroyedMethod);
        }
    });

    return reinterpret_cast<long>(recorderOpenGLProxy);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_onzhou_recorder_XWXRecorder_nativeStartPreview(JNIEnv *env, jobject instance, jlong handler,
                                                      jobject surface) {
    auto recorderOpenGLProxy = reinterpret_cast<RecorderOpenGLProxy *>(handler);
    if (recorderOpenGLProxy == nullptr) {
        return INVALID_HANDLE;
    }
    if (surface == nullptr) {
        return INVALID_HANDLE;
    }
    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
    recorderOpenGLProxy->startRender(window);
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_onzhou_recorder_XWXRecorder_updateRenderContent(JNIEnv *env, jobject instance,
                                                         jlong handler, jint texID,
                                                         jfloatArray mvp) {
    auto recorderOpenGLProxy = reinterpret_cast<RecorderOpenGLProxy *>(handler);
    if (recorderOpenGLProxy == nullptr) {
        return INVALID_HANDLE;
    }
    jfloat *mvpMatrix = env->GetFloatArrayElements(mvp, nullptr);
    recorderOpenGLProxy->updateRenderContent(static_cast<GLuint>(texID), mvpMatrix);
    env->ReleaseFloatArrayElements(mvp, mvpMatrix, 0);
    return READY;
}