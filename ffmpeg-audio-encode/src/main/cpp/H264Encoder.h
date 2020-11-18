//
// Created by xiangweixin on 2020/11/16.
//

#ifndef FFMPEG4ANDROID_MASTER_H264ENCODER_H
#define FFMPEG4ANDROID_MASTER_H264ENCODER_H

#include "XWXResult.h"
#include "logger.h"

class H264Encoder {

public:
    H264Encoder();
    ~H264Encoder();

    int open(const char *filePath);
    int addVideoStream();

private:
    AVFrame *m_pEncFrame = nullptr;
    AVPacket *m_pEncPkt = nullptr;
    AVFormatContext *m_pOutputFormatCtx = nullptr;
    FILE *m_pFile = nullptr;
};


#endif //FFMPEG4ANDROID_MASTER_H264ENCODER_H
