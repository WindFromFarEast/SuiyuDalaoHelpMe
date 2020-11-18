//
// Created by xiangweixin on 2020/11/16.
//

#ifndef FFMPEG4ANDROID_MASTER_ENCODER_H
#define FFMPEG4ANDROID_MASTER_ENCODER_H

#include "logger.h"
#include "XWXResult.h"
#include "list"
#include "string"
#include "mutex"
#include "GLData.h"

enum ENCODE_MODE {
    ABR,
    CRF,
    QP
};

/**
 * open->addStream->start->write->flush
 */
class Encoder {

public:
    Encoder();
    ~Encoder();

    int open(const char *filePath);
    int addVideoStream(int width, int height);
    int addAudioStream();
    int start();
    int writeVideoFrame(GLFrame *frame, int64_t pts);
    int writeAudioSample();
    int flushEncoder();

private:
    void cleanup();
    int writeAVPacketSafe(AVPacket *packet, AVMediaType type);
    int flushVideo();
    int flushAudio();

private:
    AVFrame *m_pVideoEncFrame = nullptr;
    AVPacket *m_pVideoEncPkt = nullptr;
    AVFormatContext *m_pOutputFormatCtx = nullptr;
    FILE *m_pFile = nullptr;
    AVStream *m_pVideoStream = nullptr;
    AVStream *m_pAudioStream = nullptr;
    AVCodecContext *m_pVideoEncCtx = nullptr;
    AVCodecContext *m_pAudioEncCtx = nullptr;
    std::list<AVPacket *> m_listVideoPkt;
    std::list<AVPacket *> m_listAudioPkt;
    std::string m_sFilePath;
    AVIOContext *m_pWriteIOCtx = nullptr;
    std::mutex m_lckWrite;
};


#endif //FFMPEG4ANDROID_MASTER_ENCODER_H
