//
// Created by xiangweixin on 2020/5/17.
//

#include "MediaPlayer.h"

void *play_video(void *args);
void *play_audio(void *args);
//解封装、初始化解码器
int MediaPlayer::init(string path, ANativeWindow *window) {
    m_path = path;
    m_window = window;

    av_register_all();

    m_fmtCtx = avformat_alloc_context();
    int ret = avformat_open_input(&m_fmtCtx, path.c_str(), nullptr, nullptr);
    if (ret < 0) {
        LOGE("avformat_open_input failed. ret: %d", ret);
        return -1;
    }
    avformat_find_stream_info(m_fmtCtx, nullptr);

    //find video & audio stream.
    for (int index = 0; index < m_fmtCtx->nb_streams; ++index) {
        if (m_fmtCtx->streams[index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            m_videoIndex = index;
        } else if (m_fmtCtx->streams[index]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            m_audioIndex = index;
        }
    }

    //config video decoder.
    m_vCodec = avcodec_find_decoder(m_fmtCtx->streams[m_videoIndex]->codecpar->codec_id);
    m_vCodecCtx = avcodec_alloc_context3(m_vCodec);
    avcodec_parameters_to_context(m_vCodecCtx, m_fmtCtx->streams[m_videoIndex]->codecpar);
    if (avcodec_open2(m_vCodecCtx, m_vCodec, nullptr) < 0) {
        LOGE("open video decoder failed.");
        return -1;
    }
    m_width = m_vCodecCtx->width;
    m_height = m_vCodecCtx->height;
    LOGI("video width: %d, video height: %d", m_width, m_height);

    //config audio decoder.
    m_aCodec = avcodec_find_decoder(m_fmtCtx->streams[m_audioIndex]->codecpar->codec_id);
    m_aCodecCtx = avcodec_alloc_context3(m_aCodec);
    avcodec_parameters_to_context(m_aCodecCtx, m_fmtCtx->streams[m_audioIndex]->codecpar);
    if (avcodec_open2(m_aCodecCtx, m_aCodec, nullptr) < 0) {
        LOGE("open audio decoder failed.");
        return -1;
    }

    pthread_mutex_init(&m_vMutex, nullptr);
    pthread_mutex_init(&m_aMutex, nullptr);
    pthread_cond_init(&m_vCond, nullptr);
    pthread_create(&m_vThreadID, nullptr, play_video, this);
    pthread_cond_init(&m_aCond, nullptr);
    pthread_create(&m_aThreadID, nullptr, play_audio, this);

    m_initialized = true;

    return 0;
}

int MediaPlayer::play() {
    pthread_cond_signal(&m_vCond);
//    pthread_cond_signal(&m_aCond);
    return 0;
}

int MediaPlayer::pause() {
    return 0;
}

int MediaPlayer::seek() {
    return 0;
}

int MediaPlayer::stop() {
    return 0;
}

#define CAST_ARGS(VAR) \
    MediaPlayer *player = reinterpret_cast<MediaPlayer *>(args);

//视频播放线程
void *play_video(void *args) {
    CAST_ARGS(args);

    pthread_cond_wait(&player->m_vCond, &player->m_vMutex);

    AVFrame *frame = av_frame_alloc();
    AVPacket *packet = static_cast<AVPacket *>(av_malloc(sizeof(AVPacket)));
    AVFrame *rgbaFrame = av_frame_alloc();

    int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA, player->m_width, player->m_height, 1);
    uint8_t *outBuffer = static_cast<uint8_t *>(av_malloc(sizeof(uint8_t) * bufferSize));
    av_image_fill_arrays(rgbaFrame->data, rgbaFrame->linesize, outBuffer, AV_PIX_FMT_RGBA, player->m_width, player->m_height, 1);
    player->m_swsCtx = sws_getContext(player->m_width, player->m_height, player->m_vCodecCtx->pix_fmt, player->m_width, player->m_height, AV_PIX_FMT_RGBA, SWS_BICUBIC,
                                      nullptr, nullptr, nullptr);

    if (ANativeWindow_setBuffersGeometry(player->m_window, player->m_width, player->m_height, WINDOW_FORMAT_RGBA_8888) < 0) {
        LOGE("ANativeWindow_setBuffersGeometry failed.");
    }

    ANativeWindow_Buffer windowBuffer;

    while (av_read_frame(player->m_fmtCtx, packet) >= 0) {
        if (packet->stream_index == player->m_videoIndex) {
            //decode video.
            if (avcodec_send_packet(player->m_vCodecCtx, packet) != 0) {
                LOGE("decode video failed.");
                break;
            }
            while (avcodec_receive_frame(player->m_vCodecCtx, frame) == 0) {
                //转化为RGBA
                sws_scale(player->m_swsCtx, (const uint8_t *const *) frame->data, frame->linesize, 0, player->m_vCodecCtx->height, rgbaFrame->data, rgbaFrame->linesize);
                if (ANativeWindow_lock(player->m_window, &windowBuffer, nullptr) < 0) {
                    LOGE("ANativeWindow_lock failed.");
                    break;
                } else {
                    uint8_t *dst = static_cast<uint8_t *>(windowBuffer.bits);
                    int dstStride = windowBuffer.stride * 4;//note: 这个stride指的是一行有多少个像素而不是多少个byte
                    uint8_t *src = rgbaFrame->data[0];
                    int srcStride = rgbaFrame->linesize[0];
                    for (int i = 0; i < player->m_height; ++i) {
                        memcpy(dst + i * dstStride, src + i * srcStride, srcStride);
                    }
                    ANativeWindow_unlockAndPost(player->m_window);
                }
            }
        }
        av_packet_unref(packet);
    }

    //release resource.
    av_frame_free(&frame);
    av_frame_free(&rgbaFrame);
    av_free(outBuffer);
    pthread_detach(player->m_vThreadID);
}

//音频播放线程
void *play_audio(void *args) {
    CAST_ARGS(args);

    pthread_cond_wait(&player->m_aCond, &player->m_aMutex);

    pthread_detach(player->m_aThreadID);
}