//
// Created by xiangweixin on 2020/11/16.
//

#include "H264Encoder.h"

H264Encoder::H264Encoder() {
    m_pEncFrame = av_frame_alloc();
    m_pEncPkt = av_packet_alloc();
}

H264Encoder::~H264Encoder() {
    av_frame_free(&m_pEncFrame);
    av_packet_free(&m_pEncPkt);
}

int H264Encoder::open(const char *filePath) {
    AVOutputFormat *outputFormat = av_guess_format("mp4", filePath, nullptr);
    if (!outputFormat) {
        LOGE("H264 Encoder open failed. av_guess_format null.");
        return FFMPEG_GUESS_FORMAT_FAILED;
    }

    const int ret = avformat_alloc_output_context2(&m_pOutputFormatCtx, outputFormat, nullptr, filePath);
    if (ret < 0) {
        LOGE("H264 Encoder open failed. avformat_alloc_output_context2 failed, ret is %d", ret);
        return FFMPEG_ALLOC_OUTPUT_FORMAT_FAILED;
    }

    m_pOutputFormatCtx->oformat->video_codec = AV_CODEC_ID_H264;
    return READY;
}

int H264Encoder::addVideoStream() {

    return READY;
}
