/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>

#ifndef MP4_STREAM_H
#define MP4_STREAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
#include "libavutil/mathematics.h"
#include "libavutil/log.h"
#include "libavutil/time.h"

#ifdef __cplusplus
}
#endif

class MP4Stream{


private:

    AVOutputFormat *ofmt = NULL;
    AVCodecContext *codec_ctx = NULL;
    AVFormatContext *in_fmt = NULL, *out_fmt = NULL;
    AVPacket avPacket;

    AVStream *in_stream = NULL, *out_stream = NULL;

    //退出标记
    int exit_flag = 1;

    int videoIndex = 0;

    int64_t start_time = 0;

public:

    int start_publish(const char *mp4Path, const char *stream);

    void stop_publish();

};


#endif
