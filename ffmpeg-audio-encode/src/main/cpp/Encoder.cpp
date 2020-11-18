//
// Created by xiangweixin on 2020/11/16.
//

#include "Encoder.h"
#include "string"
#include "iostream"
#include <unistd.h>
#include "sstream"
#include "../../../../libyuv-single/libs/armeabi-v7a/include/libyuv.h"
#include "thread"

Encoder::Encoder() {
    m_pVideoEncFrame = av_frame_alloc();
    m_pVideoEncPkt = av_packet_alloc();
}

Encoder::~Encoder() {
    av_frame_free(&m_pVideoEncFrame);
    av_packet_free(&m_pVideoEncPkt);
    cleanup();
}

int Encoder::open(const char *filePath) {
    m_sFilePath = std::string(filePath);
    AVOutputFormat *outputFormat = av_guess_format("mp4", filePath, nullptr);
    if (!outputFormat) {
        LOGE("H264 Encoder open failed. av_guess_format null.");
        cleanup();
        return FFMPEG_GUESS_FORMAT_FAILED;
    }

    const int ret = avformat_alloc_output_context2(&m_pOutputFormatCtx, outputFormat, nullptr, filePath);
    if (ret < 0) {
        LOGE("H264 Encoder open failed. avformat_alloc_output_context2 failed, ret is %d", ret);
        cleanup();
        return FFMPEG_ALLOC_OUTPUT_FORMAT_FAILED;
    }

    m_pOutputFormatCtx->oformat->video_codec = AV_CODEC_ID_H264;
    return READY;
}

int Encoder::addVideoStream(int width, int height) {
    AVCodec *videoCodec = nullptr;
    videoCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    m_pVideoStream = avformat_new_stream(m_pOutputFormatCtx, videoCodec);
    m_pVideoStream->id = 0;
    m_pVideoEncCtx = avcodec_alloc_context3(videoCodec);
    m_pVideoEncCtx->codec_id = videoCodec->id;

    m_pVideoEncCtx->framerate = {1, 30};
    m_pVideoEncCtx->time_base = AV_TIME_BASE_Q;
    m_pVideoEncCtx->profile = FF_PROFILE_H264_BASELINE;
    m_pVideoEncCtx->width = width;
    m_pVideoEncCtx->height = height;
    m_pVideoEncCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    m_pVideoEncCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    m_pVideoEncCtx->gop_size = 30;
    m_pVideoEncCtx->thread_count = av_cpu_count();
    m_pVideoEncCtx->ticks_per_frame = 2;//不知道原因
    m_pVideoEncCtx->flags |= CODEC_FLAG_CLOSED_GOP;//不知道原因
    if (videoCodec->capabilities & CODEC_CAP_FRAME_THREADS) {
        m_pVideoEncCtx->thread_type = FF_THREAD_FRAME;
    } else if (videoCodec->capabilities & CODEC_CAP_SLICE_THREADS) {
        m_pVideoEncCtx->thread_type = FF_THREAD_SLICE;
    } else {
        // NOTE: some encoder (such as libx264) support multi-threaded encoding,
        // but they don't report it in CODEC_CAP_FRAME_THREADS/CODEC_CAP_SLICE_THREADS flags
        // in that case, we prefer frame thread model
        m_pVideoEncCtx->thread_type = FF_THREAD_FRAME;
    }
    if (m_pOutputFormatCtx->oformat->flags & AVFMT_GLOBALHEADER) {
        m_pVideoEncCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }

    m_pVideoStream->avg_frame_rate = {1, 30};

    AVDictionary *param = nullptr;

    int encodeMode = ENCODE_MODE::CRF;
    if (encodeMode == CRF) {
        int crf = 15;
        av_dict_set(&param, "preset", "ultrafast", 0);
        av_dict_set(&param, "tune", "zerolatency", 0);
        std::ostringstream os;
        os << crf;
        std::string crfStr = os.str();
        av_dict_set(&param, "crf", crfStr.c_str(), 0);
    } else if (encodeMode == QP) {
        int qp = 15;
        std::ostringstream os;
        os << qp;
        av_dict_set(&param, "qp", os.str().c_str(), 0);
    } else if (encodeMode == ABR) {
        int bitrate = 9 * 1024 * 1024;
        m_pVideoEncCtx->bit_rate = bitrate;
        m_pVideoEncCtx->rc_min_rate = bitrate;
        m_pVideoEncCtx->rc_max_rate = bitrate;
        m_pVideoEncCtx->rc_buffer_size = bitrate;
    } else {

    }

    int ret = avcodec_open2(m_pVideoEncCtx, videoCodec, param ? &param : nullptr);
    if (param) {
        av_dict_free(&param);
    }
    if (ret < 0) {
        LOGE("avcodec open failed. ret is %d", ret);
        cleanup();
        return FFMPEG_CODEC_OPEN_FAILED;
    }

    ret = avcodec_parameters_from_context(m_pVideoStream->codecpar, m_pVideoEncCtx);
    if (ret < 0) {
        cleanup();
        LOGE("avcodec_parameters_from_context failed. ret is %d", ret);
        return FFMPEG_PARAMETERS_FROM_CONTEXT_FAILED;
    }

    return READY;
}

int Encoder::addAudioStream() {
    return READY;
}

int Encoder::start() {
    int ret = avio_open(&m_pOutputFormatCtx->pb, m_sFilePath.c_str(), AVIO_FLAG_WRITE);
    if (ret < 0) {
        LOGE("avio open failed. ret is %d", ret);
        cleanup();
        return FFMPEG_AVIO_OPEN_FAILED;
    }

    {
        std::lock_guard<std::mutex> lck(m_lckWrite);
        ret = avformat_write_header(m_pOutputFormatCtx, nullptr);
        if (ret < 0) {
            LOGE("avformat_write_header failed, ret is %d", ret);
            cleanup();
            return FFMPEG_WRITE_HEADER_FAILED;
        }
    }
    return READY;
}

int Encoder::writeVideoFrame(GLFrame *frame, int64_t pts) {
    if (!frame) {
        return FFMPEG_INVALID_BUFFER;
    }
    int ret;

    for (int i = 0; i < 3; ++i) {
        m_pVideoEncFrame->data[i] = static_cast<uint8_t *>(frame->buffer.data[i]);
        m_pVideoEncFrame->linesize[i] = frame->buffer.pitch[i];
    }
    m_pVideoEncFrame->width = m_pVideoEncCtx->width;
    m_pVideoEncFrame->height = m_pVideoEncCtx->height;
    m_pVideoEncFrame->format = m_pVideoEncCtx->pix_fmt;
    m_pVideoEncFrame->pts = av_rescale_q(pts, AV_TIME_BASE_Q, m_pVideoEncCtx->time_base);

    bool needReSend = false;
    do {
        av_init_packet(m_pVideoEncPkt);

        ret = avcodec_send_frame(m_pVideoEncCtx, m_pVideoEncFrame);
        if (ret == AVERROR(EAGAIN)) {
            needReSend = true;
        } else if (ret == 0) {
            needReSend = false;
        } else if (ret < 0) {
            LOGE("avcodec_send_frame video failed. ret is %d", ret);
            return FFMPEG_SEND_FRAME_UNKNOWN_ERROR;
        }

        ret = avcodec_receive_packet(m_pVideoEncCtx, m_pVideoEncPkt);
        if (ret == AVERROR(EAGAIN)) {
            break;
        } else if (ret == AVERROR_EOF) {
            LOGE("avcodec_receive_packet video failed. eof.");
            break;
        } else if (ret < 0) {
            LOGE("avcodec_receive_packet video failed. ret is %d", ret);
            return FFMPEG_RECEIVE_PACKET_UNKNOWN_ERROR;
        }

        m_pVideoEncPkt->stream_index = m_pVideoStream->index;
        m_pVideoEncPkt->pts = av_rescale_q(m_pVideoEncPkt->pts, m_pVideoEncCtx->time_base, m_pVideoStream->time_base);
        m_pVideoEncPkt->dts = av_rescale_q(m_pVideoEncPkt->dts, m_pVideoEncCtx->time_base, m_pVideoStream->time_base);
        m_pVideoEncPkt->duration = 0;

        ret = writeAVPacketSafe(m_pVideoEncPkt, AVMEDIA_TYPE_VIDEO);
        if (ret < 0) {
            break;
        }
    } while (needReSend);

    return READY;
}

int Encoder::writeAudioSample() {
    return READY;
}

int Encoder::writeAVPacketSafe(AVPacket *packet, AVMediaType type) {
    int ret = 0;
    std::lock_guard<std::mutex> lck(m_lckWrite);

    ret = av_interleaved_write_frame(m_pOutputFormatCtx, packet);
    if (ret < 0) {
        LOGE("av_interleaved_write_frame failed. MediaType: %d, ret is %d", type, ret);
    }
    if (packet != nullptr) {
        av_packet_unref(packet);
    }

    return ret;
}

int Encoder::flushEncoder() {
    int ret = 0;
    ret = flushVideo();
    if (ret < 0) {
        LOGE("flush video failed.");
    }
    ret = flushAudio();
    if (ret < 0) {
        LOGE("flush audio failed.");
    }
    ret = av_write_trailer(m_pOutputFormatCtx);
    cleanup();
    return READY;
}

int Encoder::flushVideo() {
    int maxTryCount = 10;
    int tryCount = 0;
    int ret = 0;
    bool needReSend = true;
    bool packetReceived = false;

    while (true) {
        if (needReSend) {
            ret = avcodec_send_frame(m_pVideoEncCtx, nullptr);
            if (ret  == AVERROR(EAGAIN)) {
                needReSend = true;
            } else if (ret == 0) {
                needReSend = false;
            } else if (ret < 0) {
                LOGE("avcodec_send_frame video failed. ret is %d", ret);
                return FFMPEG_SEND_FRAME_UNKNOWN_ERROR;
            }

            av_init_packet(m_pVideoEncPkt);

            ret = avcodec_receive_packet(m_pVideoEncCtx, m_pVideoEncPkt);
            if (ret == AVERROR(EAGAIN)) {
                if (!packetReceived && tryCount < maxTryCount) {
                    tryCount++;
                    LOGE("try resend and receive. tryCount: %d", tryCount);
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                } else {
                    break;
                }
            } else if (ret == AVERROR_EOF) {
                return READY;
            } else if (ret < 0) {
                return FFMPEG_RECEIVE_PACKET_UNKNOWN_ERROR;
            }

            packetReceived = true;

            m_pVideoEncPkt->stream_index = m_pVideoStream->index;
            m_pVideoEncPkt->pts = av_rescale_q(m_pVideoEncPkt->pts, m_pVideoEncCtx->time_base, m_pVideoStream->time_base);
            m_pVideoEncPkt->dts = av_rescale_q(m_pVideoEncPkt->dts, m_pVideoEncCtx->time_base, m_pVideoStream->time_base);
            m_pVideoEncPkt->duration = 0;

            ret = writeAVPacketSafe(m_pVideoEncPkt, AVMEDIA_TYPE_VIDEO);
            if (ret < 0) {
                break;
            }
        }
    }

    return READY;
}

int Encoder::flushAudio() {
    return READY;
}

void Encoder::cleanup() {
    if (m_pFile != nullptr) {
        fclose(m_pFile);
        m_pFile = nullptr;
    }
    if (!m_listVideoPkt.empty()) {
        for (auto pkt : m_listVideoPkt) {
            if (pkt) {
                av_packet_free(&pkt);
            }
        }
        m_listVideoPkt.clear();
    }
    if (!m_listAudioPkt.empty()) {
        for (auto pkt : m_listAudioPkt) {
            if (pkt) {
                av_packet_free(&pkt);
            }
        }
        m_listAudioPkt.clear();
    }
    if (m_pOutputFormatCtx) {
        if (m_pVideoEncCtx) {
            if (avcodec_is_open(m_pVideoEncCtx)) {
                avcodec_close(m_pVideoEncCtx);
            }
            avcodec_free_context(&m_pVideoEncCtx);
            m_pVideoEncCtx = nullptr;
        }
        if (m_pAudioEncCtx) {
            avcodec_free_context(&m_pAudioEncCtx);
            m_pAudioEncCtx = nullptr;
        }
        if (m_pOutputFormatCtx->pb) {
            avio_closep(&m_pOutputFormatCtx->pb);
        }
        avformat_free_context(m_pOutputFormatCtx);
        m_pOutputFormatCtx = nullptr;
    }
}
