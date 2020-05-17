
#include "native_code.h"
#include "encode_aac.h"
#include "MediaPlayer.h"
#include "logger.h"

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
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = NULL;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }
    //注册方法
    if (registerNativeMethod(env) != JNI_OK) {
        return -1;
    }
    return JNI_VERSION_1_6;
}


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