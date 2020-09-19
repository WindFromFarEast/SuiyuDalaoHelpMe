//
// Created by xiangweixin on 2020/5/17.
//

#ifndef FFMPEG4ANDROID_MASTER_MEDIAPLAYER_H
#define FFMPEG4ANDROID_MASTER_MEDIAPLAYER_H

#include <string>
#include "logger.h"
#include <pthread.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include "SLES/OpenSLES.h"
#include "SLES/OpenSLES_Android.h"
#include "SLES/OpenSLES_AndroidConfiguration.h"

using namespace std;

void *play_video(void *args);
void *play_audio(void *args);
void bufferQueueCallback(SLAndroidSimpleBufferQueueItf  slBufferQueueItf, void* context);

class MediaPlayer {

    friend void *play_video(void *args);
    friend void *play_audio(void *args);
    friend void bufferQueueCallback(SLAndroidSimpleBufferQueueItf  slBufferQueueItf, void* context);

public:
    MediaPlayer();
    ~MediaPlayer();
    int init(string path, ANativeWindow *window);
    int play();
    int pause();
    int seek();
    int stop();

private:
    int createEngine();
    int createOutputMix();
    int createPlayer();
    SLresult setPlayState(SLuint32 state);

    int getPCM(void **pcm, size_t *pcm_size);

    int m_CurrentPlayState = SL_PLAYSTATE_STOPPED;

private:
    ANativeWindow *m_window = nullptr;

    string m_path = "";
    AVFormatContext *m_fmtCtx = nullptr;
    AVCodec *m_vCodec = nullptr;
    AVCodec *m_aCodec = nullptr;
    AVCodecContext *m_vCodecCtx = nullptr;
    AVCodecContext *m_aCodecCtx = nullptr;
    SwsContext *m_swsCtx = nullptr;
    AVFrame *m_aDecodeFrame = nullptr;
    AVPacket *m_aPacket = nullptr;
    SwrContext *m_aSwrCtx = nullptr;
    uint8_t *m_aSwrOutBuffer = nullptr;

    int m_videoIndex = -1;
    int m_audioIndex = -1;
    int m_width = 0;
    int m_height = 0;

    bool m_initialized = false;

    pthread_t m_vThreadID;
    pthread_t m_aThreadID;
    pthread_mutex_t m_vMutex;
    pthread_mutex_t m_aMutex;
    pthread_cond_t m_vCond;
    pthread_cond_t m_aCond;

    //OpenSL
    SLObjectItf m_slEngineObj = nullptr;
    SLEngineItf m_slEngineItf = nullptr;
    SLObjectItf m_slOutputMixObj = nullptr;
    SLEnvironmentalReverbItf m_slEnvReverbItf = nullptr;
    SLEnvironmentalReverbSettings m_slEnvSettings = SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;
    SLObjectItf m_slPlayer = nullptr;
    SLPlayItf m_slPlayerItf = nullptr;
    SLAndroidSimpleBufferQueueItf m_slBufferQueItf = nullptr;

};


#endif //FFMPEG4ANDROID_MASTER_MEDIAPLAYER_H
