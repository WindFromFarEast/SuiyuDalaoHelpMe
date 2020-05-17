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

using namespace std;

class MediaPlayer {

    friend void *play_video(void *args);
    friend void *play_audio(void *args);

public:
    int init(string path, ANativeWindow *window);
    int play();
    int pause();
    int seek();
    int stop();

private:
    ANativeWindow *m_window = nullptr;

    string m_path = "";
    AVFormatContext *m_fmtCtx = nullptr;
    AVCodec *m_vCodec = nullptr;
    AVCodec *m_aCodec = nullptr;
    AVCodecContext *m_vCodecCtx = nullptr;
    AVCodecContext *m_aCodecCtx = nullptr;
    SwsContext *m_swsCtx = nullptr;

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

};


#endif //FFMPEG4ANDROID_MASTER_MEDIAPLAYER_H
