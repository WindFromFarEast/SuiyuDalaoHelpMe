//
// Created by byhook on 18-11-26.
//

#ifndef FFMPEG4ANDROID_LOGGER_H
#define FFMPEG4ANDROID_LOGGER_H


#ifdef ANDROID

#include <android/log.h>
#include <libavutil/time.h>

extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include "libavfilter/avfilter.h"
#include "libavfilter/avfiltergraph.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <unistd.h>

#ifdef __cplusplus
}

#define LOG_TAG    "NativeAudioEncode"
#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, "XWXDebug", format, ##__VA_ARGS__)
#define LOGI(format, ...)  __android_log_print(ANDROID_LOG_INFO,  "XWXDebug", format, ##__VA_ARGS__)
#define LOGD(format, ...)  __android_log_print(ANDROID_LOG_DEBUG,  "XWXDebug", format, ##__VA_ARGS__)
#else
#define LOGE(format, ...)  printf(LOG_TAG format "\n", ##__VA_ARGS__)
#define LOGI(format, ...)  printf(LOG_TAG format "\n", ##__VA_ARGS__)
#endif

#endif //FFMPEG4ANDROID_LOGGER_H
