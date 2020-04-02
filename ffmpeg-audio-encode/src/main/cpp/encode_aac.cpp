
#include "encode_aac.h"
#include "logger.h"

int AACEncoder::EncodeFrame(AVCodecContext *pCodecCtx, AVFrame *audioFrame) {
    int64_t pts = (int64_t)(1000000L * (frameCount++) * 1024.0F / audioStream->codec->sample_rate);
    if (audioFrame != nullptr) {
        audioFrame->pts = av_rescale_q(pts, (AVRational) { 1, AV_TIME_BASE }, audioStream->time_base);
    }
    int ret = avcodec_send_frame(pCodecCtx, audioFrame);
    if (ret < 0) {
        LOGE("Could't send frame");
        return -1;
    }
    while (avcodec_receive_packet(pCodecCtx, &audioPacket) == 0) {
        audioPacket.stream_index = audioStream->index;
        ret = av_write_frame(pFormatCtx, &audioPacket);
        av_packet_unref(&audioPacket);
    }
    return ret;
}

int AACEncoder::EncodeStart(const char *aacPath) {
    //1.注册所有组件
    av_register_all();
    //2.获取输出文件的上下文环境
    int ret = avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, aacPath);
    if (ret < 0 || pFormatCtx == nullptr) {
        return -1;
    }
    fmt = pFormatCtx->oformat;
    //3.打开输出文件
    if ((ret = avio_open(&pFormatCtx->pb, aacPath, AVIO_FLAG_READ_WRITE)) < 0) {
        LOGE("Could't open output file, %d", ret);
        return ret;
    }
    //4.新建音频流
    audioStream = avformat_new_stream(pFormatCtx, NULL);
    audioStream->time_base = {1, 44100};
    if (audioStream == NULL) {
        return -1;
    }
    //5.寻找编码器并打开编码器
    pCodec = avcodec_find_encoder_by_name("libfdk_aac");
    if (pCodec == NULL) {
        LOGE("Could not find encoder");
        return -1;
    }

    //6.分配编码器并设置参数
    pCodecCtx = audioStream->codec;
    pCodecCtx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    pCodecCtx->codec_id = pCodec->id;
    pCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;
    pCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16;
    pCodecCtx->sample_rate = 44100;
    pCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
    pCodecCtx->channels = av_get_channel_layout_nb_channels(pCodecCtx->channel_layout);
    pCodecCtx->bit_rate = 44100 * 2 * 2;
    if (pFormatCtx->oformat->flags & AVFMT_GLOBALHEADER) {
        pCodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }

    //7.打开音频编码器
    int result = avcodec_open2(pCodecCtx, pCodec, NULL);
    if (result < 0) {
        LOGE("Could't open encoder, result: %d", result);
        char errorMessage[1024] = {0};
        av_strerror(1024, errorMessage, sizeof(errorMessage));
        LOGE("error message: %s", errorMessage);
        return -1;
    }

    audioFrame = av_frame_alloc();
    audioFrame->nb_samples = pCodecCtx->frame_size;
    audioFrame->format = pCodecCtx->sample_fmt;

    bufferSize = av_samples_get_buffer_size(NULL, pCodecCtx->channels, pCodecCtx->frame_size,
                                            pCodecCtx->sample_fmt, 1);
    LOGE("channels: %d, frame_size: %d, sample_fmt: %d, bufferSize: %d", pCodecCtx->channels, pCodecCtx->frame_size,
         pCodecCtx->sample_fmt, bufferSize);
    audioBuffer = (uint8_t *) av_malloc(bufferSize);
    avcodec_fill_audio_frame(audioFrame, pCodecCtx->channels, pCodecCtx->sample_fmt,
                             (const uint8_t *) audioBuffer, bufferSize, 1);


    //8.写文件头
    avformat_write_header(pFormatCtx, NULL);
    av_new_packet(&audioPacket, bufferSize);

    return 0;
}

int AACEncoder::EncodeBuffer(uint8_t *data, int length) {
    int ret = 0;
    int originLength = length;
    int bufferSize = av_samples_get_buffer_size(nullptr, audioStream->codec->channels, audioStream->codec->frame_size, audioStream->codec->sample_fmt, 1);
    uint8_t *audioBuffer = static_cast<uint8_t *>(av_malloc(bufferSize));
    avcodec_fill_audio_frame(audioFrame, audioStream->codec->channels, audioStream->codec->sample_fmt, audioBuffer, bufferSize, 1);

    audioFrame->format = audioStream->codec->sample_fmt;
    audioFrame->nb_samples = audioStream->codec->frame_size;

    uint8_t *combiningData = nullptr;
    // encode first audio frame. check whether length > bufferSize or not.
    if (length > bufferSize && mCachePcmData == nullptr && mFirstMp4Encode) {
        mCachePcmData = static_cast<uint8_t *>(av_malloc(length));
    }

    if (mCachePcmData != nullptr) {
        combiningData = static_cast<uint8_t *>(av_malloc(bufferSize));
    }

    if (mCachePcmData != nullptr) {

        if (mFirstMp4Encode || mCachePcmDataLength == 0) {
            //编码第一帧，此时无缓存PCM数据
            audioFrame->data[0] = data;
            ret = EncodeFrame(pCodecCtx, audioFrame);

            length = length - bufferSize;

            int firstEncodeTimes = 1;

            while ((length - bufferSize) >= 0) {
                memcpy(combiningData, data + firstEncodeTimes * bufferSize, bufferSize * sizeof(uint8_t));
                audioFrame->data[0] = combiningData;
                ret = EncodeFrame(pCodecCtx, audioFrame);
                length = length - bufferSize;
                firstEncodeTimes++;
            }

            memcpy(mCachePcmData, data + (firstEncodeTimes * bufferSize), (originLength - (firstEncodeTimes * bufferSize)) * sizeof(uint8_t));
            mCachePcmDataLength = originLength - (firstEncodeTimes * bufferSize);
            LOGE("cache length: %d", mCachePcmDataLength);
        } else {
            //不是第一帧，需要使用上一次剩下的PCM数据
            memcpy(combiningData, mCachePcmData, mCachePcmDataLength * sizeof(uint8_t));
            memcpy(combiningData + mCachePcmDataLength, data, (bufferSize - mCachePcmDataLength) * sizeof(uint8_t));
            int curDataPos = bufferSize - mCachePcmDataLength;
            audioFrame->data[0] = combiningData;

            ret = EncodeFrame(pCodecCtx, audioFrame);
            length = length - (bufferSize - mCachePcmDataLength);

            while ((length - bufferSize) >= 0) {
                memcpy(combiningData, data + curDataPos, bufferSize * sizeof(uint8_t));
                audioFrame->data[0] = combiningData;
                ret = EncodeFrame(pCodecCtx, audioFrame);
                length = length - bufferSize;
                curDataPos += bufferSize;
            }

            memcpy(mCachePcmData, data, length * sizeof(uint8_t));
            mCachePcmDataLength = length;
            LOGE("cache length: %d", mCachePcmDataLength);
        }

    } else {
        ret = EncodeFrame(pCodecCtx, audioFrame);
    }

    if (combiningData != nullptr) {
        av_free(combiningData);
    }

    mFirstMp4Encode = false;
    av_free(audioBuffer);

    return ret;
}

int AACEncoder::EncodeStop() {

    EncodeFrame(pCodecCtx, nullptr);
    //10.写文件尾
    av_write_trailer(pFormatCtx);

    //释放
    avcodec_close(pCodecCtx);
    av_free(audioFrame);
    av_free(audioBuffer);
    avio_close(pFormatCtx->pb);
    avformat_free_context(pFormatCtx);

    mFirstMp4Encode = true;
    frameCount = 0;
    mCachePcmDataLength = 0;
    return 0;
}


