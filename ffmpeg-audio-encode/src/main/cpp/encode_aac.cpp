
#include "encode_aac.h"
#include "logger.h"

int AACEncoder::EncodeFrame(AVCodecContext *pCodecCtx, AVFrame *audioFrame) {
    int ret = 0;
    AVFrame *frame = nullptr;
    if (audioFrame != nullptr) {
        audioFrame->nb_samples = 1024;
        audioFrame->channels = 2;
        audioFrame->channel_layout = pCodecCtx->channel_layout;
        audioFrame->sample_rate = 44100;
        audioFrame->format = AV_SAMPLE_FMT_S16;
        int ret = av_buffersrc_add_frame_flags(m_pBufferSrcCtx, audioFrame, 0);
        if (ret < 0) {
            return ret;
        }
        frame = av_frame_alloc();
        ret = av_buffersink_get_frame(m_pBufferSinkCtx, frame);
        if (ret < 0) {
            return ret;
        }
        int64_t pts = (int64_t)((1000000L * (frameCount++) * 1024.0F / audioStream->codec->sample_rate) / 2.0F);
        if (frame != nullptr) {
            frame->pts = av_rescale_q(pts, (AVRational) { 1, AV_TIME_BASE }, audioStream->time_base);
        }
        ret = avcodec_send_frame(pCodecCtx, frame);
        if (ret < 0) {
            LOGE("Could't send frame");
            return -1;
        }
    }
    while (avcodec_receive_packet(pCodecCtx, &audioPacket) == 0) {
        audioPacket.stream_index = audioStream->index;
        ret = av_write_frame(pFormatCtx, &audioPacket);
        av_packet_unref(&audioPacket);
    }
    if (frame != nullptr) {
        av_frame_free(&frame);
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
                             (const uint8_t *) audioBuffer, bufferSize, 0);


    //8.写文件头
    avformat_write_header(pFormatCtx, NULL);
    av_new_packet(&audioPacket, bufferSize);

    initAudioFilter();

    return 0;
}

int AACEncoder::EncodeBuffer(uint8_t *data, int length) {
    int ret = 0;
    int originLength = length;
    bufferSize = av_samples_get_buffer_size(nullptr, audioStream->codec->channels, audioStream->codec->frame_size, audioStream->codec->sample_fmt, 1);
    LOGE("bufferSize: %d", bufferSize);
    if (mFirstMp4Encode) {
        audioFrame->data[0] = audioBuffer;
    }

    audioFrame->format = audioStream->codec->sample_fmt;
    audioFrame->nb_samples = audioStream->codec->frame_size;

    // encode first audio frame. check whether length > bufferSize or not.
    if (length > bufferSize && mCachePcmData == nullptr && mFirstMp4Encode) {
        mCachePcmData = static_cast<uint8_t *>(av_malloc(bufferSize));
    }

    if (mCachePcmData != nullptr) {
        int encodedDataLength = 0;
        while (true) {
            if (length + mCachePcmDataLength >= bufferSize) {
                // PCM Data里剩下的数据+缓存数据能够组成bufferSize作为一帧编码
                memcpy(mCachePcmData + mCachePcmDataLength, data + encodedDataLength, (bufferSize - mCachePcmDataLength) * sizeof(uint8_t));
                encodedDataLength += (bufferSize - mCachePcmDataLength);
                memcpy(audioFrame->data[0], mCachePcmData, bufferSize * sizeof(uint8_t));
                ret = EncodeFrame(pCodecCtx, audioFrame);
                length = length - (bufferSize - mCachePcmDataLength);
                mCachePcmDataLength = 0;
            } else {
                memcpy(mCachePcmData, data + encodedDataLength, length * sizeof(uint8_t));
                mCachePcmDataLength += length;
                break;
            }
        }
    } else {
        memcpy(audioFrame->data[0], data, length * sizeof(uint8_t));
        ret = EncodeFrame(pCodecCtx, audioFrame);
    }

    mFirstMp4Encode = false;

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

int AACEncoder::initAudioFilter() {
    av_register_all();
    avcodec_register_all();
    avfilter_register_all();
    snprintf(m_filter_descr, sizeof(m_filter_descr), "atempo=%.2lf", 2.0F);
    char args[512];
    int ret = 0;
    m_abuffersrc  = avfilter_get_by_name("abuffer");
    m_abuffersink = avfilter_get_by_name("abuffersink");
    m_inputs = avfilter_inout_alloc();
    m_outputs  = avfilter_inout_alloc();
    AVSampleFormat out_sample_fmts[] = { AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE };
    int64_t out_channel_layouts[] = { AV_CH_LAYOUT_STEREO, -1 };
    int out_sample_rates[] = { 44100, -1 };
    const AVFilterLink *outlink;
    AVRational time_base = pCodecCtx->time_base;
    m_pFilterGraph = avfilter_graph_alloc();
    if (!m_inputs || !m_outputs || !m_pFilterGraph) {
        ret = AVERROR(ENOMEM);
        goto end;
    }
    /* buffer audio source: the decoded frames from the decoder will be inserted here. */
    snprintf(args, sizeof(args),
             "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%#x",
             time_base.num, time_base.den, pCodecCtx->sample_rate,
             av_get_sample_fmt_name(pCodecCtx->sample_fmt), (int) av_get_default_channel_layout(pCodecCtx->channels));
    ret = avfilter_graph_create_filter(&m_pBufferSrcCtx, m_abuffersrc, "in",
                                       args, NULL, m_pFilterGraph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer source\n");
        goto end;
    }
    /* buffer audio sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&m_pBufferSinkCtx, m_abuffersink, "out",
                                       NULL, NULL, m_pFilterGraph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer sink\n");
        goto end;
    }
    ret = av_opt_set_int_list(m_pBufferSinkCtx, "sample_fmts", out_sample_fmts, -1,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output sample format\n");
        goto end;
    }
    ret = av_opt_set_int_list(m_pBufferSinkCtx, "channel_layouts", out_channel_layouts, -1,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output channel layout\n");
        goto end;
    }
    ret = av_opt_set_int_list(m_pBufferSinkCtx, "sample_rates", out_sample_rates, -1,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output sample rate\n");
        goto end;
    }
    /*
     * Set the endpoints for the filter graph. The filter_graph will
     * be linked to the graph described by filters_descr.
     */
    /*
     * The buffer source output must be connected to the input pad of
     * the first filter described by filters_descr; since the first
     * filter input label is not specified, it is set to "in" by
     * default.
     */
    m_outputs->name       = av_strdup("in");
    m_outputs->filter_ctx = m_pBufferSrcCtx;
    m_outputs->pad_idx    = 0;
    m_outputs->next       = NULL;
    /*
     * The buffer sink input must be connected to the output pad of
     * the last filter described by filters_descr; since the last
     * filter output label is not specified, it is set to "out" by
     * default.
     */
    m_inputs->name       = av_strdup("out");
    m_inputs->filter_ctx = m_pBufferSinkCtx;
    m_inputs->pad_idx    = 0;
    m_inputs->next       = NULL;
    if ((ret = avfilter_graph_parse_ptr(m_pFilterGraph, m_filter_descr,
                                        &m_inputs, &m_outputs, NULL)) < 0)
        goto end;
    if ((ret = avfilter_graph_config(m_pFilterGraph, NULL)) < 0)
        goto end;
    /* Print summary of the sink buffer
     * Note: args buffer is reused to store channel layout string */
    outlink = m_pBufferSinkCtx->inputs[0];
    av_get_channel_layout_string(args, sizeof(args), -1, outlink->channel_layout);
//    av_log(NULL, AV_LOG_INFO, "Output: srate:%dHz fmt:%s chlayout:%s\n",
//           (int)outlink->sample_rate,
//           (char *)av_x_if_null(av_get_sample_fmt_name(outlink->format), "?"),
//           args);
    end:
    avfilter_inout_free(&m_inputs);
    avfilter_inout_free(&m_outputs);
    return ret;

}


