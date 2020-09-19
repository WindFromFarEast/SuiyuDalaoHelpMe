//
// Created by xiangweixin on 2020/5/17.
//

#include "MediaPlayer.h"

#define CHECK_OPENSL_RESULT(VAL) \
    if(VAL != SL_RESULT_SUCCESS) { \
        LOGE("%s:%d failed. ret: %d", __FUNCTION__, __LINE__, VAL); \
        return VAL; \
    }

MediaPlayer::MediaPlayer() {
    m_aDecodeFrame = av_frame_alloc();
    m_aPacket = av_packet_alloc();
    m_aSwrCtx = swr_alloc();
}

MediaPlayer::~MediaPlayer() {
    av_frame_free(&m_aDecodeFrame);
    av_packet_free(&m_aPacket);
    swr_free(&m_aSwrCtx);
    delete []m_aSwrOutBuffer;
}

//解封装、初始化解码器、OpenSL引擎等
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

    int64_t out_ch_layout = AV_CH_LAYOUT_STEREO;//输出声道布局为立体声
    AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;//输出采样格式/采样精度为16位=2字节
    int out_sample_rate = m_aCodecCtx->sample_rate;
    int out_nb_channels = av_get_channel_layout_nb_channels(out_ch_layout);
    int64_t in_ch_layout = av_get_default_channel_layout(m_aCodecCtx->channels);
    AVSampleFormat in_sample_fmt = m_aCodecCtx->sample_fmt;
    int in_sample_rate = m_aCodecCtx->sample_rate;

    swr_alloc_set_opts(m_aSwrCtx, out_ch_layout, out_sample_fmt, out_sample_rate, in_ch_layout, in_sample_fmt, in_sample_rate, 0,
                       nullptr);
    swr_init(m_aSwrCtx);
    /**
    * 存放重采样后的数据缓冲区 , 这个缓冲区存储 1 秒的数据
    * 44100 Hz 采样率 , 16 位采样位数 , 双声道立体声 , 占用内存 44100 * 2 * 2 字节
    */
    m_aSwrOutBuffer = new uint8_t[44100 * 2 * 2];


    //init OpenSL ES..
    ret = createEngine();
    ret = createOutputMix();
    ret = createPlayer();

    pthread_mutex_init(&m_vMutex, nullptr);
    pthread_mutex_init(&m_aMutex, nullptr);
    pthread_cond_init(&m_vCond, nullptr);
    pthread_create(&m_vThreadID, nullptr, play_video, this);
    pthread_cond_init(&m_aCond, nullptr);
    pthread_create(&m_aThreadID, nullptr, play_audio, this);

    m_initialized = true;

    return ret;
}

int MediaPlayer::createEngine() {
    //1、获取OpenSL引擎接口
    SLresult ret = slCreateEngine(&m_slEngineObj, 0, nullptr, 0, nullptr, nullptr);
    CHECK_OPENSL_RESULT(ret);
    //2、通过OpenSL引擎接口获取引擎具体对象，并初始化
    ret = (*m_slEngineObj)->Realize(m_slEngineObj, SL_BOOLEAN_FALSE);//实现引擎接口对象
    CHECK_OPENSL_RESULT(ret);
    ret = (*m_slEngineObj)->GetInterface(m_slEngineObj, SL_IID_ENGINE, &m_slEngineItf);//通过引擎接口对象调用接口初始化SLEngineItf(引擎具体对象)
    CHECK_OPENSL_RESULT(ret);

    return ret;
}

int MediaPlayer::createOutputMix() {
    //3、通过引擎具体对象创建混音器接口
    SLresult ret;
    ret = (*m_slEngineItf)->CreateOutputMix(m_slEngineItf, &m_slOutputMixObj, 0, nullptr, nullptr);
    CHECK_OPENSL_RESULT(ret);
    //4、通过混音器接口获取具体的混音器接口对象，并初始化具体的混音环境
    ret = (*m_slOutputMixObj)->Realize(m_slOutputMixObj, SL_BOOLEAN_FALSE);
    CHECK_OPENSL_RESULT(ret);
    ret = (*m_slOutputMixObj)->GetInterface(m_slOutputMixObj, SL_IID_ENVIRONMENTALREVERB, &m_slEnvReverbItf);//获取混音器具体的对象
    CHECK_OPENSL_RESULT(ret);
    ret = (*m_slEnvReverbItf)->SetEnvironmentalReverbProperties(m_slEnvReverbItf, &m_slEnvSettings);
    CHECK_OPENSL_RESULT(ret);

    return ret;
}

int MediaPlayer::createPlayer() {
    /**
     * typedef struct SLDataLocator_AndroidBufferQueue_ {
        SLuint32    locatorType;
        SLuint32    numBuffers;  //todo 该参数是什么意思
      }  SLDataLocator_AndroidBufferQueue;
     */
    SLDataLocator_AndroidBufferQueue slBufferQueue = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2 };
    SLDataFormat_PCM pcm = {
            SL_DATAFORMAT_PCM,
            2,//声道数
            44100 * 1000,//采样率
            SL_PCMSAMPLEFORMAT_FIXED_16,//采样位数
            SL_PCMSAMPLEFORMAT_FIXED_16,//包含位数
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,//立体声
            SL_BYTEORDER_LITTLEENDIAN //小端
    };
    SLDataSource dataSource = {
            &slBufferQueue,
            &pcm
    };
    SLDataLocator_OutputMix slDataLocatorMix = {
            SL_DATALOCATOR_OUTPUTMIX,
            m_slOutputMixObj
    };
    SLDataSink slDataSink = {
            &slDataLocatorMix,
            nullptr
    };

    const SLInterfaceID ids[3] = {
            SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_VOLUME, SL_IID_ANDROIDCONFIGURATION
    };
    SLboolean req[3]={
            SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE
    };

    //5、通过引擎具体对象创建播放器接口
    SLresult ret;
    ret = (*m_slEngineItf)->CreateAudioPlayer(m_slEngineItf, &m_slPlayer, &dataSource, &slDataSink,
                                              sizeof(ids) / sizeof(ids[0]), ids, req);
    CHECK_OPENSL_RESULT(ret);

    ret = (*m_slPlayer)->Realize(m_slPlayer, SL_BOOLEAN_FALSE);
    CHECK_OPENSL_RESULT(ret);

    //6、通过播放器接口初始化播放器具体对象
    ret = (*m_slPlayer)->GetInterface(m_slPlayer, SL_IID_PLAY, &m_slPlayerItf);
    CHECK_OPENSL_RESULT(ret);

    //7、注册队列缓冲区，通过缓冲区里面的数据进行播放
    ret = (*m_slPlayer)->GetInterface(m_slPlayer, SL_IID_BUFFERQUEUE, &m_slBufferQueItf);
    CHECK_OPENSL_RESULT(ret);

    //8、设置回调接口
    ret = (*m_slBufferQueItf)->RegisterCallback(m_slBufferQueItf, bufferQueueCallback, this);
    CHECK_OPENSL_RESULT(ret);

    return ret;
}

SLresult MediaPlayer::setPlayState(SLuint32 state) {
    if (m_slPlayerItf != nullptr) {
       SLresult ret = (*m_slPlayerItf)->SetPlayState(m_slPlayerItf, state);
       CHECK_OPENSL_RESULT(ret);
       m_CurrentPlayState = state;
       return ret;
    }
    LOGE("Set play state failed. m_slPlayerItf is nullptr!");
    return SL_RESULT_UNKNOWN_ERROR;
}

int MediaPlayer::play() {
    pthread_cond_signal(&m_vCond);
    pthread_cond_signal(&m_aCond);
    return 0;
}

int MediaPlayer::pause() {
    return 0;
}

int MediaPlayer::seek() {
    return 0;
}

int MediaPlayer::stop() {
    //释放资源 OpenSL、FFmpeg
    return 0;
}

int MediaPlayer::getPCM(void **pcm, size_t *pcm_size) {
    int decodeRet = -1;
    bool shouldResend = false;

    while (1) {
        if (!shouldResend) {
            decodeRet = av_read_frame(m_fmtCtx, m_aPacket);
            if (decodeRet < 0) {
                *pcm = nullptr;
                *pcm_size = 0;
            }
            if (m_aPacket->stream_index != m_audioIndex) {
                av_packet_unref(m_aPacket);
                continue;
            }
        }
        decodeRet = avcodec_send_packet(m_aCodecCtx, m_aPacket);
        if (decodeRet == AVERROR(EAGAIN)) {
            shouldResend = true;
        } else if (decodeRet == AVERROR_EOF) {
            av_packet_unref(m_aPacket);
        } else if (decodeRet < 0) {
            av_packet_unref(m_aPacket);
            LOGE("音频解码send异常. error code: %d", decodeRet);
        } else if (decodeRet == 0) {
            shouldResend = false;
            av_packet_unref(m_aPacket);
        }

        decodeRet = avcodec_receive_frame(m_aCodecCtx, m_aDecodeFrame);
        if (decodeRet == AVERROR(EAGAIN)) {
            continue;
        } else if (decodeRet == AVERROR_EOF) {
            LOGI("音频解码结束.");
            *pcm = nullptr;
            *pcm_size = 0;
            break;
        } else if (decodeRet < 0) {
            LOGE("音频解码receive异常. error code: %d", decodeRet);
            *pcm = nullptr;
            *pcm_size = 0;
            break;
        }

        LOGD("音频解码了一帧.");
        /**
         * 这里传入样本采样率 , 获取的是延迟的样本个数
         */
        int64_t delay = swr_get_delay(m_aSwrCtx, m_aDecodeFrame->sample_rate);
        /**
         * int64_t av_rescale_rnd(int64_t a, int64_t b, int64_t c, enum AVRounding rnd) ----> a * b / c.
         * 以 "时钟基c" 表示的 数值a 转换成以 "时钟基b" 来表示。
         */
         /**
          * 输出音频采样数 = 输入音频采样数 * 输出音频采样率 / 输入音频采样率.
          */
        int64_t out_count = av_rescale_rnd(m_aDecodeFrame->nb_samples + delay, 44100, m_aDecodeFrame->sample_rate, AV_ROUND_UP);
        swr_convert(m_aSwrCtx, &m_aSwrOutBuffer, out_count, (const uint8_t **) m_aDecodeFrame->data, m_aDecodeFrame->nb_samples);

        *pcm_size = static_cast<size_t>(av_samples_get_buffer_size(nullptr, 2, out_count,
                                                                   AVSampleFormat::AV_SAMPLE_FMT_S16,
                                                                   1));
        *pcm = new uint8_t[*pcm_size];
        memcpy(*pcm, m_aSwrOutBuffer, *pcm_size);
        break;
    }

    return decodeRet;

}

#define CAST_ARGS(VAR) \
    MediaPlayer *player = reinterpret_cast<MediaPlayer *>(VAR);

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

    return nullptr;
}

//音频解码线程
void *play_audio(void *args) {
    CAST_ARGS(args);

    pthread_cond_wait(&player->m_aCond, &player->m_aMutex);

    player->setPlayState(SL_PLAYSTATE_PLAYING);
    bufferQueueCallback(player->m_slBufferQueItf, player);

    pthread_detach(player->m_aThreadID);
    return nullptr;
}

void bufferQueueCallback(SLAndroidSimpleBufferQueueItf  slBufferQueueItf, void* context) {
    CAST_ARGS(context);

    LOGD("buffer callback.");
    void *buffer = nullptr;
    size_t bufferSize = 0;
    int ret = player->getPCM(&buffer, &bufferSize);

    if (ret >= 0 && buffer != nullptr && bufferSize > 0) {
        LOGD("play audio. bufferSize: %d", bufferSize);
        (*player->m_slBufferQueItf)->Enqueue(player->m_slBufferQueItf, buffer, bufferSize);
    }

    LOGD("buffer callback done.");
}
