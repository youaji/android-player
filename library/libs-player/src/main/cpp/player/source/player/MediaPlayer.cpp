#include "MediaPlayer.h"

/**
 * FFmpeg 操作锁管理回调
 * @param mtx 这是一个二级指针，即指针的指针，传递二级指针的目的是为了改变一级指针的值
 * @param op
 * @return
 */
static int lockmgrCallback(void **mtx, enum AVLockOp op) {
    switch (op) {
        case AV_LOCK_CREATE: {
            *mtx = new Mutex();
            if (!*mtx) {
                LOGD("MediaPlayer->failed to create mutex.");
                return 1;
            }
            return 0;
        }

        case AV_LOCK_OBTAIN: {
            if (!*mtx) {
                return 1;
            }
            return ((Mutex *) (*mtx))->lock() != 0;
        }

        case AV_LOCK_RELEASE: {
            if (!*mtx) {
                return 1;
            }
            return ((Mutex *) (*mtx))->unlock() != 0;
        }

        case AV_LOCK_DESTROY: {
            if (!*mtx) {
                delete (*mtx);
                *mtx = NULL;
            }
            return 0;
        }
    }
    return 1;
}

MediaPlayer::MediaPlayer() {
    av_register_all();
    avformat_network_init();
    // 主要用来保存播放器的信息
    mPlayerState = new PlayerState();
    mDuration = -1;
    mAudioDecoder = NULL;
    mVideoDecoder = NULL;
    mFormatCtx = NULL;
    mLastPaused = -1;
    mAttachmentRequest = 0;

#if defined(__ANDROID__)
    mAudioDevice = new SLESDevice();
#else
    mAudioDevice = new AudioDevice();
#endif

    mMediaSync = new MediaSync(mPlayerState);
    mAudioResampler = NULL;
    mReadThread = NULL;
    mVideoRecorder = NULL;
//    mScreenshotRecorder = NULL;
    mIsExit = true;

    // 注册一个多线程锁管理回调，主要是解决多个视频源时保持 avcodec_open/close 的原子操作
    if (av_lockmgr_register(lockmgrCallback)) {
        LOGE("MediaPlayer->Could not initialize lock manager!");
    }

}

MediaPlayer::~MediaPlayer() {
    LOGD("MediaPlayer->播放器析构");
    avformat_network_deinit();
    av_lockmgr_register(NULL);
}

status_t MediaPlayer::reset() {
    // 先停止
    stop();
    if (mMediaSync) {
        mMediaSync->reset();
        delete mMediaSync;
        mMediaSync = NULL;
    }
    if (mAudioDecoder != NULL) {
        mAudioDecoder->stop();
        delete mAudioDecoder;
        mAudioDecoder = NULL;
    }
    if (mVideoDecoder != NULL) {
        mVideoDecoder->stop();
        delete mVideoDecoder;
        mVideoDecoder = NULL;
    }
    if (mAudioDevice != NULL) {
        mAudioDevice->stop();
        delete mAudioDevice;
        mAudioDevice = NULL;
    }
    if (mAudioResampler) {
        delete mAudioResampler;
        mAudioResampler = NULL;
    }
    if (mFormatCtx != NULL) {
        avformat_close_input(&mFormatCtx);
        avformat_free_context(mFormatCtx);
        mFormatCtx = NULL;
    }
    if (mPlayerState) {
        delete mPlayerState;
        mPlayerState = NULL;
    }
    return NO_ERROR;
}

void MediaPlayer::setDataSource(const char *url, int64_t offset, const char *headers) {
    // 因为 Autolock 属于一个局部变量，在这里执行了lock上锁操作，当方法执行完，这个局部变量要销毁，会执行析构函数，而在析构函数中会执行解锁操作unlock
    Mutex::Autolock lock(mMutex);
    mPlayerState->url = av_strdup(url); //拷贝字符串
    mPlayerState->offset = offset; //文件偏移量
    if (headers) { //一般没有headers
        mPlayerState->headers = av_strdup(headers);
    }
}


void MediaPlayer::setVideoDevice(VideoDevice *videoDevice) {
    Mutex::Autolock lock(mMutex);
    mMediaSync->setVideoDevice(videoDevice);
}

status_t MediaPlayer::prepare() {
    Mutex::Autolock lock(mMutex);
    if (!mPlayerState->url) {
        return BAD_VALUE;
    }
    mPlayerState->abort_request = 0;
    LOGD("MediaPlayer->准备播放");
    // 开启读数据线程准备
    if (!mReadThread) {
        LOGD("MediaPlayer->准备播放 --- 初始化线程并开始播放");
        mReadThread = new Thread(this); // 因为当前类继承了runnable
        mReadThread->start();
    }
    return NO_ERROR;
}

status_t MediaPlayer::prepareAsync() {
    Mutex::Autolock lock(mMutex);
    if (!mPlayerState->url) {
        return BAD_VALUE;
    }
    // 发送消息
    if (mPlayerState->message_queue) {
        // 异步的意思就是在工作线程中去准备，这里首先发送一个MSG_REQUEST_PREPARE出去，在YouajiMediaPlayer类处理消息的线程中收到消息以后，
        // 再调用prepare方法，这样就实现了在接收消息的工作线程中去执行prepare方法
        mPlayerState->message_queue->postMessage(MSG_REQUEST_PREPARE);
    }
    return NO_ERROR;
}

void MediaPlayer::start() {
    Mutex::Autolock lock(mMutex);
    mPlayerState->abort_request = 0;
    mPlayerState->pause_request = 0;
    mIsExit = false;
    mCondition.signal(); //通知
}

void MediaPlayer::pause() {
    Mutex::Autolock lock(mMutex);
    mPlayerState->pause_request = 1;
    mCondition.signal();
}

void MediaPlayer::resume() {
    Mutex::Autolock lock(mMutex);
    mPlayerState->pause_request = 0;
    mCondition.signal();
}

void MediaPlayer::stop() {
    mMutex.lock();
    mPlayerState->abort_request = 1;
    mCondition.signal();
    mMutex.unlock();

    mMutex.lock();
    while (!mIsExit) {
        mCondition.wait(mMutex);
    }
    mMutex.unlock();

    /*停止消息线程*/
    if (mPlayerState->message_queue) {
        mPlayerState->message_queue->stop();
    }

    if (mReadThread != NULL) {
        mReadThread->join();
        delete mReadThread;
        LOGD("MediaPlayer->删除播放线程");
        mReadThread = NULL;
    }

    if (mVideoRecorder != NULL) {

    }
}

void MediaPlayer::seekTo(float timeMs) {
    // when is a live media stream, duration is -1
    if (!mPlayerState->real_time && mDuration < 0) {
        return;
    }

    // 等待上一次操作完成
    mMutex.lock();
    while (mPlayerState->seek_request) {
        mCondition.wait(mMutex);
    }
    mMutex.unlock();

    if (!mPlayerState->seek_request) {
        int64_t start_time = 0;
        int64_t seek_pos = av_rescale(timeMs, AV_TIME_BASE, 1000);
        start_time = mFormatCtx ? mFormatCtx->start_time : 0;
        if (start_time > 0 && start_time != AV_NOPTS_VALUE) {
            seek_pos += start_time;
        }
        // 设置定位位置
        mPlayerState->seek_pos = seek_pos;
        mPlayerState->seek_rel = 0;
        mPlayerState->seek_flags &= ~AVSEEK_FLAG_BYTE;
        // 有定位请求
        mPlayerState->seek_request = 1;
        mCondition.signal();
    }

}

void MediaPlayer::setLooping(int looping) {
    mMutex.lock();
    mPlayerState->loop = looping;
    mCondition.signal();
    mMutex.unlock();
}

void MediaPlayer::setVolume(float volume) {
    if (mAudioDevice) {
        mAudioDevice->setVolume(volume);
    }
}

float MediaPlayer::getVolume() {
    if (mAudioDevice) {
        return mAudioDevice->getVolume();
    }
    return 0;
}

void MediaPlayer::setStereoVolume(float leftVolume, float rightVolume) {
    if (mAudioDevice) {
        mAudioDevice->setStereoVolume(leftVolume, rightVolume);
    }
}

void MediaPlayer::setMute(int mute) {
    mMutex.lock();
    mPlayerState->mute = mute;
    mCondition.signal();
    mMutex.unlock();
}

void MediaPlayer::setRate(float rate) {
    mMutex.lock();
    mPlayerState->playback_rate = rate;
    mCondition.signal();
    mMutex.unlock();
}

void MediaPlayer::setPitch(float pitch) {
    mMutex.lock();
    mPlayerState->playback_pitch = pitch;
    mCondition.signal();
    mMutex.unlock();
}

int MediaPlayer::getRotate() {
    Mutex::Autolock lock(mMutex);
    if (mVideoDecoder) {
        return mVideoDecoder->getRotate();
    }
    return 0;
}

int MediaPlayer::getVideoWidth() {
    Mutex::Autolock lock(mMutex);
    if (mVideoDecoder) {
        return mVideoDecoder->getCodecContext()->width;
    }
    return 0;
}

int MediaPlayer::getVideoHeight() {
    Mutex::Autolock lock(mMutex);
    if (mVideoDecoder) {
        return mVideoDecoder->getCodecContext()->height;
    }
    return 0;
}

long MediaPlayer::getCurrentPosition() {
    Mutex::Autolock lock(mMutex);
    int64_t currentPosition = 0;
    // 处于定位
    if (mPlayerState->seek_request) {
        currentPosition = mPlayerState->seek_pos;
    } else {

        // 起始延时
        int64_t start_time = mFormatCtx->start_time;

        int64_t start_diff = 0;
        if (start_time > 0 && start_time != AV_NOPTS_VALUE) {
            start_diff = av_rescale(start_time, 1000, AV_TIME_BASE);
        }

        // 计算主时钟的时间
        int64_t pos = 0;
        double clock = mMediaSync->getMasterClock();
        if (isnan(clock)) {
            pos = mPlayerState->seek_pos;
        } else {
            pos = (int64_t) (clock * 1000);
        }
        if (pos < 0 || pos < start_diff) {
            return 0;
        }
        return (long) (pos - start_diff);
    }
    return (long) currentPosition;
}

long MediaPlayer::getDuration() {
    Mutex::Autolock lock(mMutex);
    return (long) mDuration;
}

int MediaPlayer::isPlaying() {
    Mutex::Autolock lock(mMutex);
    return !mPlayerState->abort_request && !mPlayerState->pause_request;
}

int MediaPlayer::isLooping() {
    return mPlayerState->loop;
}

int MediaPlayer::getMetadata(AVDictionary **metadata) {
    if (!mFormatCtx) {
        return -1;
    }
    // TODO getMetadata
    return NO_ERROR;
}

static int avformat_interrupt_cb(void *ctx) {
    PlayerState *playerState = (PlayerState *) ctx;
    if (playerState->abort_request) {
        return AVERROR_EOF;
    }
    return 0;
}

AVMessageQueue *MediaPlayer::getMessageQueue() {
    Mutex::Autolock lock(mMutex);
    return mPlayerState->message_queue;
}

PlayerState *MediaPlayer::getPlayerState() {
    Mutex::Autolock lock(mMutex);
    return mPlayerState;
}

void MediaPlayer::run() {
    startPlayer();
}

void MediaPlayer::notifyErrorMessage(const char *message) {
    if (mPlayerState->message_queue) {
        mPlayerState->message_queue->postMessage(MSG_ERROR, 0, 0, (void *) message,
                                                 sizeof(message) / message[0]);
    }
}

/**
 * 关键函数，开始播放
 * @return
 */
int MediaPlayer::startPlayer() {
    int ret = 0;

    /* 1、解封装 */
    ret = demux();
    // 解封装失败
    if (ret < 0) {
        mIsExit = true;
        mCondition.signal();
        notifyErrorMessage("demux failed!");
        return ret; // 返回
    }

    /* 2、准备解码器 */
    ret = prepareDecoder();
    if (ret < 0) {
        mIsExit = true;
        mCondition.signal();
        notifyErrorMessage("prepare decoder failed!");
        return ret; // 返回
    }

    /* 3、开始解码 */
    startDecode();

    /* 4、打开多媒体播放设备 */
    ret = openMediaDevice();
    if (ret < 0) {
        mIsExit = true;
        mCondition.signal();
        notifyErrorMessage("open device failed!");
        return ret; // 返回
    }

    /* 5、获取packet数据 */
    ret = readAVPackets();
    if (ret < 0 && ret != AVERROR_EOF) { // 播放出错
        notifyErrorMessage("error when reading packets!");
    }

    // 停止消息队列
    if (mPlayerState->message_queue) {
        mPlayerState->message_queue->stop();
    }

    return ret;
}

/**
 * 解封装和准备音视频解码器
 * @return
 */
int MediaPlayer::demux() {
    int ret = 0;
    AVDictionaryEntry *t;
    AVDictionary **opts;
    int scan_all_pmts_set = 0;

    // 线程加锁
    mMutex.lock();

    do {
        // 解封装功能结构体
        mFormatCtx = avformat_alloc_context();

        if (!mFormatCtx) {
            LOGE("MediaPlayer->Could not allocate context.");
            ret = AVERROR(ENOMEM); //内存不足错误
            break;
        }

        // 设置解封装中断回调
        mFormatCtx->interrupt_callback.callback = avformat_interrupt_cb;
        mFormatCtx->interrupt_callback.opaque = mPlayerState; //回调函数的参数

        // 设置解封装option参数，AV_DICT_MATCH_CASE是精准匹配key值，第二个参数是key值
        // 设置 scan_all_pmts 应该只是针对 avformat_open_input() 函数用的。之后就又设为 NULL
        if (!av_dict_get(mPlayerState->format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE)) { //是否有设置了参数
            // scan_all_pmts 的特殊处理为基于 ffplay 开发的软件提供了一个“很好的错误示范”，导致经常看到针对特定编码或封装的特殊选项、特殊处理充满了 read_thread
            // 影响代码可读性！
            av_dict_set(&mPlayerState->format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE); //设置参数
            scan_all_pmts_set = 1;
        }

        // 处理文件头
        if (mPlayerState->headers) { //一般没有头文件
            av_dict_set(&mPlayerState->format_opts, "headers", mPlayerState->headers, 0);
        }

        // 处理文件偏移量
        if (mPlayerState->offset > 0) {
            mFormatCtx->skip_initial_bytes = mPlayerState->offset;
        }

        // 设置 rtmp/rtsp 的超时值
        if (av_stristart(mPlayerState->url, "rtmp", NULL) ||
            av_stristart(mPlayerState->url, "rtsp", NULL)) {
            // There is total different meaning for 'timeout' option in rtmp
            LOGW("MediaPlayer->remove 'timeout' option for rtmp.");
            av_dict_set(&mPlayerState->format_opts, "timeout", NULL, 0); // 设置为null
        }

        // 参数说明：
        // AVFormatContext **ps, 格式化的上下文。要注意，如果传入的是一个 AVFormatContext* 的指针，则该空间须自己手动清理，若传入的指针为空，则FFmpeg会内部自己创建
        // const char *url, 传入的地址。支持http,RTSP,以及普通的本地文件。地址最终会存入到AVFormatContext结构体当中
        // AVInputFormat *fmt, 指定输入的封装格式。一般传NULL，由FFmpeg自行探测
        // AVDictionary **options, 其它参数设置。它是一个字典，用于参数传递，不传则写NULL。参见：libavformat/options_table.h,其中包含了它支持的参数设置
        /*打开文件*/
        ret = avformat_open_input(&mFormatCtx, mPlayerState->url, mPlayerState->input_format, &mPlayerState->format_opts);
        // LOGE("MediaPlayer->字典的大小%d",mPlayerState->format_opts->count)
        if (ret < 0) {
            printError(mPlayerState->url, ret);
            ret = -1;
            break;
        }
        LOGD("MediaPlayer->open url success[%s]", mPlayerState->url);

        // 通知文件已打开
        if (mPlayerState->message_queue) {
            mPlayerState->message_queue->postMessage(MSG_OPEN_INPUT);
        }

        // 又设置为null
        if (scan_all_pmts_set) {
            av_dict_set(&mPlayerState->format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE);
        }

        // 上面设置以后，字典大小已经为null了
        // LOGE("MediaPlayer->字典的大小%d",mPlayerState->format_opts->count)
        // 迭代字典，""是所有字符串的前缀，如果有条目，则获取到字典中的第一个条目
        if ((t = av_dict_get(mPlayerState->format_opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) { // 这里应该返回null，即字典里没有条目了
            LOGE("MediaPlayer->Option %s not found.", t->key);
            ret = AVERROR_OPTION_NOT_FOUND;
            break;
        }

        if (mPlayerState->genpts) {
            mFormatCtx->flags |= AVFMT_FLAG_GENPTS;
        }

        // 插入全局的附加信息
        av_format_inject_global_side_data(mFormatCtx);
        // 设置媒体流信息参数
        opts = setupStreamInfoOptions(mFormatCtx, mPlayerState->codec_opts);

        /* 查找媒体流信息 */
        ret = avformat_find_stream_info(mFormatCtx, opts);
        // 释放字典内存
        if (opts != NULL) {
            for (int i = 0; i < mFormatCtx->nb_streams; i++) {
                if (opts[i] != NULL) {
                    av_dict_free(&opts[i]);
                }
            }
            av_freep(&opts);
        }

        if (ret < 0) {
            LOGW("MediaPlayer->%s: could not find codec parameters", mPlayerState->url);
            ret = -1;
            break;
        }

        // 查找媒体流信息回调
        if (mPlayerState->message_queue) {
            mPlayerState->message_queue->postMessage(MSG_FIND_STREAM_INFO);
        }

        // 判断是否实时流，判断是否需要设置无限缓冲区
        mPlayerState->real_time = isRealTime(mFormatCtx);
        // 如果是实时流和偏移量 < 0
        if (mPlayerState->infinite_buffer < 0 && mPlayerState->real_time) {
            mPlayerState->infinite_buffer = 1;
        }

        // Gets the duration of the file, -1 if no duration available.
        // 获取文件的持续时间，如果没有可用的持续时间，则获取 -1。
        if (mPlayerState->real_time) { // 如果实时流，视频就没有时长，比如直播就属于实时流
            mDuration = -1;
        } else {
            /* 计算视频时长 */
            mDuration = -1;
            if (mFormatCtx->duration != AV_NOPTS_VALUE) {
                // a * b / c 函数表示在b下的占a个格子，在c下是多少
                mDuration = av_rescale(mFormatCtx->duration, 1000, AV_TIME_BASE); // 视频时长，单位毫秒
            }
        }

        // 设置av时长
        mPlayerState->video_duration = mDuration;
        // AVIOContext *pb，IO上下文，自定义格式读/从内存当中读
        if (mFormatCtx->pb) {
            mFormatCtx->pb->eof_reached = 0; // 是否读到文件尾
        }

        // 判断是否以字节方式定位，!!取两次反，是为了把非0值转换成1,而0值还是0
        mPlayerState->seek_by_bytes = !!(mFormatCtx->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", mFormatCtx->iformat->name);

        // 设置最大帧间隔
        mMediaSync->setMaxDuration((mFormatCtx->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0);

        // 如果不是从头开始播放，则跳转到播放位置
        if (mPlayerState->start_time != AV_NOPTS_VALUE) { // AV_NOPTS_VALUE表示没有时间戳的意思，一般刚开始第一帧是AV_NOPTS_VALUE的
            int64_t timestamp;

            timestamp = mPlayerState->start_time;
            if (mFormatCtx->start_time != AV_NOPTS_VALUE) {
                timestamp += mFormatCtx->start_time;
            }
            // 加锁
            mPlayerState->mutex.lock();
            // 视频时间定位
            ret = avformat_seek_file(mFormatCtx, -1, INT64_MIN, timestamp, INT64_MAX, 0);
            mPlayerState->mutex.unlock();
            if (ret < 0) {
                LOGW("MediaPlayer->%s: could not seek to position %0.3f", mPlayerState->url, (double) timestamp / AV_TIME_BASE);
            }
        }

    } while (false);
    mMutex.unlock();

    /*返回结果*/
    return ret;
}

/**
 * 准备解码器
 * @return
 */
int MediaPlayer::prepareDecoder() {
    int ret = 0;
    mMutex.lock();

    // 查找媒体流索引
    int audioIndex = -1;
    int videoIndex = -1;
    for (int i = 0; i < mFormatCtx->nb_streams; ++i) {
        if (mFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (audioIndex == -1) {
                audioIndex = i;
            }
        } else if (mFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (videoIndex == -1) {
                videoIndex = i;
            }
        }
    }

    // 如果不禁止视频流，则查找最合适的视频流索引
    if (!mPlayerState->video_disable) {
        videoIndex = av_find_best_stream(mFormatCtx, AVMEDIA_TYPE_VIDEO, videoIndex, -1, NULL, 0);
    } else {
        videoIndex = -1;
    }

    // 如果不禁止音频流，则查找最合适的音频流索引(与视频流关联的音频流)
    if (!mPlayerState->audio_disable) {
        audioIndex = av_find_best_stream(mFormatCtx, AVMEDIA_TYPE_AUDIO, audioIndex, videoIndex, NULL, 0);
    } else {
        audioIndex = -1;
    }

    // 如果音频流和视频流都没有找到，则直接退出
    if (audioIndex == -1 && videoIndex == -1) {
        LOGW("MediaPlayer->%s: could not find audio and video stream", mPlayerState->url);
        ret = -1;
        return ret;
    }

    /* 准备视频和音频的解码器 */
    if (audioIndex >= 0) {
        prepareDecoder(audioIndex);
    }
    if (videoIndex >= 0) {
        prepareDecoder(videoIndex);
    }

    // 创建音视频解码器失败
    if (!mAudioDecoder && !mVideoDecoder) {
        LOGW("MediaPlayer->failed to create audio and video decoder");
        ret = -1;
        return ret;
    }
    // 准备解码器消息通知
    if (mPlayerState->message_queue) {
        mPlayerState->message_queue->postMessage(MSG_PREPARE_DECODER);
    }

    mMutex.unlock();
    return ret;
}

void MediaPlayer::startDecode() {
    // 视频解码器
    if (mVideoDecoder) {
        AVCodecParameters *codecpar = mVideoDecoder->getStream()->codecpar;
        // 通知播放视频的宽高
        if (mPlayerState->message_queue) {
            mPlayerState->message_queue->postMessage(MSG_VIDEO_SIZE_CHANGED, codecpar->width, codecpar->height);
            mPlayerState->message_queue->postMessage(MSG_SAR_CHANGED, codecpar->sample_aspect_ratio.num, codecpar->sample_aspect_ratio.den);
        }
    }

    // 准备完成的通知
    if (mPlayerState->message_queue) {
        mPlayerState->message_queue->postMessage(MSG_PREPARED);
    }

    if (mVideoDecoder != NULL) {
        /* 视频解码器开始解码 */
        mVideoDecoder->start();
        // 对外通知解码已开始
        if (mPlayerState->message_queue) {
            mPlayerState->message_queue->postMessage(MSG_VIDEO_START);
        }
    } else {
        if (mPlayerState->sync_type == AV_SYNC_VIDEO) {
            mPlayerState->sync_type = AV_SYNC_AUDIO;
        }
    }

    if (mAudioDecoder != NULL) {
        /* 音频解码器开始解码 */
        mAudioDecoder->start();
        if (mPlayerState->message_queue) {
            mPlayerState->message_queue->postMessage(MSG_AUDIO_START);
        }
    } else {
        if (mPlayerState->sync_type == AV_SYNC_AUDIO) {
            mPlayerState->sync_type = AV_SYNC_EXTERNAL;
        }
    }
}

/**
 * 打开多媒体播放设备
 * @return
 */
int MediaPlayer::openMediaDevice() {
    int ret = 0;
    // 打开音频输出设备
    if (mAudioDecoder != NULL) {
        LOGD("MediaPlayer->打开音频设备");
        AVCodecContext *avctx = mAudioDecoder->getCodecContext(); // 解码上下文
        // 打开音频设备
        ret = openAudioDevice(avctx->channel_layout, avctx->channels, avctx->sample_rate);
        if (ret < 0) {
            LOGW("MediaPlayer->could not open audio device");
            // 如果音频设备打开失败，则调整时钟的同步类型
            if (mPlayerState->sync_type == AV_SYNC_AUDIO) {
                if (mVideoDecoder != NULL) {
                    mPlayerState->sync_type = AV_SYNC_VIDEO;
                } else {
                    mPlayerState->sync_type = AV_SYNC_EXTERNAL;
                }
            }
        } else {
            // 启动音频输出设备
            mAudioDevice->start();
        }
    }

    if (mVideoDecoder) {
        if (mPlayerState->sync_type == AV_SYNC_AUDIO) { // 默认的
            // 设置主时钟为音频时钟
            mVideoDecoder->setMasterClock(mMediaSync->getAudioClock());
        } else if (mPlayerState->sync_type == AV_SYNC_VIDEO) {
            mVideoDecoder->setMasterClock(mMediaSync->getVideoClock());
        } else {
            mVideoDecoder->setMasterClock(mMediaSync->getExternalClock());
        }
    }

    /*开始视频的同步播放*/
    mMediaSync->start(mVideoDecoder, mAudioDecoder);

    /*等待开始，当 mediaPlayer 调用 start 或 resume 方法的时候，pauseRequest 就为 true*/
    if (mPlayerState->pause_request) { // 刚开始的时候，即还准备了但还没调用start的时候，会调用PlayerState的reset方法，那里设置了pauseRequest=1
        // 请求开始
        if (mPlayerState->message_queue) {
            mPlayerState->message_queue->postMessage(MSG_REQUEST_START);
        }
        // 循环等待开始
        while ((!mPlayerState->abort_request) && mPlayerState->pause_request) {
            av_usleep(10 * 1000);
        }
    }

    if (mPlayerState->message_queue) {
        mPlayerState->message_queue->postMessage(MSG_STARTED);
    }

    return ret;
}

/**
 * 读取av数据
 * @return
 */
int MediaPlayer::readAVPackets() {
    // 读数据包流程
    mEOF = 0;
    int ret = 0;
    AVPacket pkt1, *pkt = &pkt1;
    int64_t stream_start_time;
    int playInRange = 0;
    int64_t pkt_ts;
    int waitToSeek = 0;

//    int frame_index = 0;//统计帧数
//
    /* 循环读取数据包压入队列，以供播放音视频 */
    for (;;) {

        // 退出播放器
        if (mPlayerState->abort_request) {
            break;
        }

        // 是否暂停网络流
        if (mPlayerState->pause_request != mLastPaused) {
            mLastPaused = mPlayerState->pause_request;
            if (mPlayerState->pause_request) { // 暂停
                av_read_pause(mFormatCtx);
            } else {
                av_read_play(mFormatCtx);
            }
        }

#if CONFIG_RTSP_DEMUXER || CONFIG_MMSH_PROTOCOL
        if (playerState->pauseRequest &&
            (!strcmp(mFormatCtx->inputFormat->name, "rtsp") ||
             (mFormatCtx->pb && !strncmp(url, "mmsh:", 5)))) {
            av_usleep(10 * 1000);
            continue;
        }
#endif
        /* 处理定位请求 */
        if (mPlayerState->seek_request) {
            int64_t seek_target = mPlayerState->seek_pos;
            // seekRel 默认为 0
            int64_t seek_min =
                    mPlayerState->seek_rel > 0 ? seek_target - mPlayerState->seek_rel + 2 : INT64_MIN;
            int64_t seek_max =
                    mPlayerState->seek_rel < 0 ? seek_target - mPlayerState->seek_rel - 2 : INT64_MAX;
            // 定位
            mPlayerState->mutex.lock();
            // avformat_seek_file定位
            ret = avformat_seek_file(
                    mFormatCtx,
                    -1,
                    seek_min,
                    seek_target,
                    seek_max,
                    mPlayerState->seek_flags
            );
            mPlayerState->mutex.unlock();
            if (ret < 0) {
                LOGE("MediaPlayer->%s: error while seeking", mPlayerState->url);
            } else {
                // 定位时清空原来的缓冲区
                if (mAudioDecoder) {
                    mAudioDecoder->flush();
                }
                if (mVideoDecoder) {
                    mVideoDecoder->flush();
                }

                // 更新外部时钟值
                if (mPlayerState->seek_flags & AVSEEK_FLAG_BYTE) {
                    mMediaSync->updateExternalClock(NAN);
                } else {
                    mMediaSync->updateExternalClock(seek_target / (double) AV_TIME_BASE);
                }
                // 更新视频帧的计时器
                mMediaSync->refreshVideoTimer();
            }

            mAttachmentRequest = 1;
            mPlayerState->seek_request = 0;
            mCondition.signal();
            mEOF = 0;
            // 定位完成回调通知
            if (mPlayerState->message_queue) {
                mPlayerState->message_queue->postMessage(
                        MSG_SEEK_COMPLETE,
                        (int) av_rescale(seek_target, 1000, AV_TIME_BASE),
                        ret
                );
            }
        }

        // 取得封面数据包
        if (mAttachmentRequest) {
            if (mVideoDecoder
                && (mVideoDecoder->getStream()->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
                AVPacket copy;
                // 该函数用于复制 AVPacket 中的 buf 和 data，如果使用了计数 buffer AVBufferRef,
                // 则将 AVBufferRef 中的数据空间计数加一，不复制其他成员
                if ((ret = av_packet_ref(&copy, &mVideoDecoder->getStream()->attached_pic)) < 0) { // 附加pic
                    break;
                }
                mVideoDecoder->pushPacket(&copy);
            }
            mAttachmentRequest = 0;
        }

        /* 暂停会在这里循环 */
        // 如果队列中存在足够的数据包，则等待消耗
        // 备注：这里要等待一定时长的缓冲队列，要不然会导致 OpenSLES 播放音频出现卡顿等现象
        // 暂停的时候，也会一直执行里面的 continue，因为队列满了但没消耗
        if (mPlayerState->infinite_buffer < 1 &&
            ((mAudioDecoder ? mAudioDecoder->getMemorySize() : 0) + (mVideoDecoder ? mVideoDecoder->getMemorySize() : 0) > MAX_QUEUE_SIZE
             || (!mAudioDecoder || mAudioDecoder->hasEnoughPackets()) && (!mVideoDecoder || mVideoDecoder->hasEnoughPackets()))) {
            // 当播放器执行暂停的时候，也会不断执行这里，
            // 因为暂停的时候音视频就会停止消耗数据，然后音视频队列就会超过最大值从而等待
            // 然后也就暂停了从文件中读取数据
            continue;
        }

        /* 读取数据包 */
        if (!waitToSeek) { // 没有等待定位
            ret = av_read_frame(mFormatCtx, pkt); //返回0即为OK，小于0就是出错了或者读到了文件的结尾
        } else {
            ret = -1;
        }

        /* 出错或者文件读完了 */
        // 获取读文件的返回值ret，成功ret等于0，否则为负数
        if (ret < 0) {
            // 如果没能读出数据包，判断是否是结尾
            if ((ret == AVERROR_EOF || avio_feof(mFormatCtx->pb)) && !mEOF) {
                // 通知播放完成
                if (mPlayerState->message_queue) {
                    mPlayerState->message_queue->postMessage(MSG_COMPLETED);
                }
                mEOF = 1;
            }
            // 读取出错，则直接退出，退出for循环
            if (mFormatCtx->pb && mFormatCtx->pb->error) {
                ret = -1;
                break;
            }

            // 如果不处于暂停状态，并且队列中没有数据
            // 有几个情况：循环播放、自动退出、播放完毕
            if (!mPlayerState->pause_request && (!mAudioDecoder || mAudioDecoder->getPacketSize() == 0)
                && (!mVideoDecoder || (mVideoDecoder->getPacketSize() == 0 && mVideoDecoder->getFrameSize() == 0))) {

                if (mPlayerState->loop) { // 循环播放
                    seekTo(mPlayerState->start_time != AV_NOPTS_VALUE ? mPlayerState->start_time : 0); // 定位到开始位置循环播放
                } else if (mPlayerState->auto_exit) { // 自动退出
                    ret = AVERROR_EOF;
                    break;
                }
//                else if (eof == 1) { // 播放完毕退出
//                    break;
//                }
            }
            // 睡眠10毫秒
            av_usleep(10 * 1000);
            // 如果读取失败或者播放完毕以后还不退出，则继续下一个循环
            continue; // 如果不退出就来个空循环
        } else {
            mEOF = 0;
        }

        // 计算 pkt 的 pts 是否处于播放范围内
        stream_start_time = mFormatCtx->streams[pkt->stream_index]->start_time;
        pkt_ts = pkt->pts == AV_NOPTS_VALUE ? pkt->dts : pkt->pts; // 当前packet的时间戳
        // 是否在播放范围
        playInRange = mPlayerState->duration == AV_NOPTS_VALUE ||
                      (pkt_ts - (stream_start_time != AV_NOPTS_VALUE ? stream_start_time : 0)) *
                      av_q2d(mFormatCtx->streams[pkt->stream_index]->time_base) -
                      (double) (mPlayerState->start_time != AV_NOPTS_VALUE ? mPlayerState->start_time : 0) / 1000000
                      <= ((double) mPlayerState->duration / 1000000);

//        mPlayerState->mutex.lock();
//        if (mVideoRecorder != NULL && mOutputCtx != NULL) {
//            if (mVideoRecorder->isRecording) {
//                AVStream *inputStream = mFormatCtx->streams[pkt->stream_index];
//                AVStream *outputStream = mOutputCtx->streams[pkt->stream_index];
//
//                if (inputStream && outputStream) {
//                    // 处理同步
//                    av_packet_rescale_ts(pkt, inputStream->time_base, outputStream->time_base);
//
//                    pkt->dts = 0;
//                    if (pkt->pts == AV_NOPTS_VALUE) {
//                        //Write PTS时间 刻度
//                        AVRational time_base1 = inputStream->time_base;
//
//                        //Duration between 2 frames (us) 时长
//                        //AV_TIME_BASE 时间基
//                        //av_q2d(AVRational);该函数负责把AVRational结构转换成double，通过这个函数可以计算出某一帧在视频中的时间位置
//                        //r_frame_rate
//                        int64_t calc_duration = (double) AV_TIME_BASE / av_q2d(inputStream->r_frame_rate);
//                        //Parameters参数
//                        pkt->pts = (double) (frame_index * calc_duration) / (double) (av_q2d(time_base1) * AV_TIME_BASE);
//                        pkt->dts = pkt->pts;
//                        pkt->duration = (double) calc_duration / (double) (av_q2d(time_base1) * AV_TIME_BASE);
//                        frame_index++;
//                    }
//                    pkt->pts = av_rescale_q_rnd(pkt->pts, inputStream->time_base, outputStream->time_base, (enum AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
//                    pkt->dts = av_rescale_q_rnd(pkt->dts, inputStream->time_base, outputStream->time_base, (enum AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
//                    pkt->duration = av_rescale_q(pkt->duration, inputStream->time_base, outputStream->time_base);
//                    pkt->pos = -1;
//
//                    // 写入数据到输出文件
//                    ret = av_interleaved_write_frame(mOutputCtx, pkt);
//                    LOGD("MediaPlayer->VideoRecorder->pts=%lld, dts=%lld", pkt->pts, pkt->dts);
//                    if (ret < 0) {
//                        LOGE("MediaPlayer->VideoRecorder->write a packet to an output media file failed,err=%d", ret);
//                    }
//                }
//            }
//        }
//        mPlayerState->mutex.unlock();
//
        /* 将音频或者视频数据包压入队列 */
        if (playInRange && mAudioDecoder && pkt->stream_index == mAudioDecoder->getStreamIndex()) {
            mAudioDecoder->pushPacket(pkt);
        } else if (playInRange && mVideoDecoder && pkt->stream_index == mVideoDecoder->getStreamIndex()) {
            mVideoDecoder->pushPacket(pkt);
        } else {
            av_packet_unref(pkt);
        }
        // 等待定位
        if (!playInRange) {
            waitToSeek = 1;
        }
    }

    LOGD("MediaPlayer->循环结束");

    if (mAudioDecoder) {
        mAudioDecoder->stop();
    }
    if (mVideoDecoder) {
        mVideoDecoder->stop();
    }
    if (mAudioDevice) {
        mAudioDevice->stop();
    }
    if (mMediaSync) {
        mMediaSync->stop();
    }
    // 主要作用是当线程执行完毕，才退出，关键是上面几个stop也执行了
    mIsExit = true;
    mCondition.signal();
    return ret;
}


int MediaPlayer::prepareDecoder(int streamIndex) {
    AVCodecContext *avctx; // 解码上下文
    AVCodec *codec = NULL; // 解码器
    AVDictionary *opts = NULL; // 参数字典
    AVDictionaryEntry *t = NULL; // 字典条目
    int ret = 0;
    const char *forcedCodecName = NULL;

    if (streamIndex < 0 || streamIndex >= mFormatCtx->nb_streams) {
        return -1;
    }

    /* 创建解码上下文 */
    avctx = avcodec_alloc_context3(NULL);
    if (!avctx) {
        return AVERROR(ENOMEM);
    }

    do {
        /* 复制解码上下文参数 */
        ret = avcodec_parameters_to_context(avctx, mFormatCtx->streams[streamIndex]->codecpar);
        if (ret < 0) {
            break;
        }

        // 设置时钟基准
        av_codec_set_pkt_timebase(avctx, mFormatCtx->streams[streamIndex]->time_base);

        // 优先使用指定的解码器
        switch (avctx->codec_type) {
            case AVMEDIA_TYPE_AUDIO: {
                LOGD("MediaPlayer->音频时间基 %d, %d", mFormatCtx->streams[streamIndex]->time_base.den,
                     mFormatCtx->streams[streamIndex]->time_base.num);
                forcedCodecName = mPlayerState->audio_codec_name;
                break;
            }
            case AVMEDIA_TYPE_VIDEO: {
                LOGD("MediaPlayer->视频时间基 %d, %d", mFormatCtx->streams[streamIndex]->time_base.den,
                     mFormatCtx->streams[streamIndex]->time_base.num);
                forcedCodecName = mPlayerState->video_codec_name;
                break;
            }
            case AVMEDIA_TYPE_UNKNOWN:
                break;
            case AVMEDIA_TYPE_DATA:
                break;
            case AVMEDIA_TYPE_SUBTITLE:
                break;
            case AVMEDIA_TYPE_ATTACHMENT:
                break;
            case AVMEDIA_TYPE_NB:
                break;
        }

        // 通过解码器名字去查找
        if (forcedCodecName) {
            LOGD("MediaPlayer->forceCodecName = %s", forcedCodecName);
            codec = avcodec_find_decoder_by_name(forcedCodecName);
        }

        // 如果没有找到指定的解码器，则查找默认的解码器
        if (!codec) {
            if (forcedCodecName) {
                LOGW("MediaPlayer->No codec could be found with name '%s'", forcedCodecName);
            }
            /*查找解码器*/
            codec = avcodec_find_decoder(avctx->codec_id);
        }

        // 判断是否成功得到解码器
        if (!codec) {
            LOGW("MediaPlayer->No codec could be found with id %d", avctx->codec_id);
            ret = AVERROR(EINVAL);
            break;
        }

        // 设置解码器id
        avctx->codec_id = codec->id;

        // 设置一些播放参数
        int stream_lowres = mPlayerState->lowres; // 解码器支持的最低分辨率
        if (stream_lowres > av_codec_get_max_lowres(codec)) {
            LOGW("MediaPlayer->The maximum value for lowres supported by the decoder is %d", av_codec_get_max_lowres(codec));
            stream_lowres = av_codec_get_max_lowres(codec);
        }
        av_codec_set_lowres(avctx, stream_lowres);
#if FF_API_EMU_EDGE
        if (stream_lowres) {
            avctx->flags |= CODEC_FLAG_EMU_EDGE;
        }
#endif
        if (mPlayerState->fast) {
            avctx->flags2 |= AV_CODEC_FLAG2_FAST;
        }

#if FF_API_EMU_EDGE
        if (codec->capabilities & AV_CODEC_CAP_DR1) {
            avctx->flags |= CODEC_FLAG_EMU_EDGE;
        }
#endif

        // 过滤解码参数
        opts = filterCodecOptions(
                mPlayerState->codec_opts,
                avctx->codec_id,
                mFormatCtx,
                mFormatCtx->streams[streamIndex],
                codec
        );
        if (!av_dict_get(opts, "threads", NULL, 0)) {
            av_dict_set(&opts, "threads", "auto", 0);
        }

        if (stream_lowres) {
            av_dict_set_int(&opts, "lowres", stream_lowres, 0);
        }

        if (avctx->codec_type == AVMEDIA_TYPE_VIDEO || avctx->codec_type == AVMEDIA_TYPE_AUDIO) {
            av_dict_set(&opts, "refcounted_frames", "1", 0);
        }

        /*打开解码器*/
        if ((ret = avcodec_open2(avctx, codec, &opts)) < 0) {
            break;
        }
        if ((t = av_dict_get(opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
            LOGE("MediaPlayer->Option %s not found.", t->key);
            ret = AVERROR_OPTION_NOT_FOUND;
            break;
        }

        // 根据解码器类型创建解码器
        mFormatCtx->streams[streamIndex]->discard = AVDISCARD_DEFAULT; // 抛弃无用的数据比如像0大小的packet

        /*根据解码器类型，创建对应的解码器*/
        switch (avctx->codec_type) {
            case AVMEDIA_TYPE_AUDIO: {
                if (mAudioDecoder == NULL) {
                    mAudioDecoder = new AudioDecoder(avctx, mFormatCtx->streams[streamIndex], streamIndex, mPlayerState);
                }
                // 如果已经有解码器了，就重置解码器的参数
                break;
            }

            case AVMEDIA_TYPE_VIDEO: {
                if (mVideoDecoder == NULL) {
                    mVideoDecoder = new VideoDecoder(mFormatCtx, avctx, mFormatCtx->streams[streamIndex], streamIndex, mPlayerState);
                }
                mAttachmentRequest = 1;
                break;
            }

            default: {
                break;
            }
        }
    } while (false);

    // 准备失败，则需要释放创建的解码上下文
    if (ret < 0) {
        if (mPlayerState->message_queue) {
            const char errorMsg[] = "failed to open stream!";
            mPlayerState->message_queue->postMessage(MSG_ERROR, 0, 0, (void *) errorMsg, sizeof(errorMsg) / errorMsg[0]);
        }
        // 失败了就要释放解码上下文
        avcodec_free_context(&avctx);
    }
    // 释放参数
    av_dict_free(&opts);
    return ret;
}

/**
 * 音频取pcm数据的回调方法
 * @param opaque
 * @param stream
 * @param len
 */
void audioPCMQueueCallback(void *opaque, uint8_t *stream, int len) {
    MediaPlayer *mediaPlayer = (MediaPlayer *) opaque;
    mediaPlayer->pcmQueueCallback(stream, len);
}

/**
 * 打开音频设备
 * @param wanted_channel_layout 声道分布
 * @param wanted_nb_channels 声道数量
 * @param wanted_sample_rate 采样率
 * @return
 */
int MediaPlayer::openAudioDevice(int64_t wanted_channel_layout, int wanted_nb_channels, int wanted_sample_rate) {
    AudioDeviceSpec wanted_spec, spec;
    const int next_nb_channels[] = {0, 0, 1, 6, 2, 6, 4, 6};
    const int next_sample_rates[] = {44100, 48000};
    int next_sample_rate_idx = FF_ARRAY_ELEMS(next_sample_rates) - 1; //最后一个index开始
    // av_get_channel_layout_nb_channels 为根据通道 layout 返回通道个数
    if (wanted_nb_channels != av_get_channel_layout_nb_channels(wanted_channel_layout) || !wanted_channel_layout) {
        wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels); //获取默认的通道layout
        wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
    }
    // 期望的声道数
    wanted_nb_channels = av_get_channel_layout_nb_channels(wanted_channel_layout);
    wanted_spec.channels = wanted_nb_channels;
    wanted_spec.freq = wanted_sample_rate;

    if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {
        LOGE("MediaPlayer->Invalid sample rate or channel count!");
        return -1;
    }

    // 找到在next_sample_rates数组中比wanted_spec.freq小的那个采样率的index
    while (next_sample_rate_idx && next_sample_rates[next_sample_rate_idx] >= wanted_spec.freq) {
        next_sample_rate_idx--;
    }

    wanted_spec.format = AV_SAMPLE_FMT_S16;
    wanted_spec.samples = FFMAX(AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(wanted_spec.freq / AUDIO_MAX_CALLBACKS_PER_SEC));

    // 设置音频取数据的回调方法
    wanted_spec.callback = audioPCMQueueCallback;
    wanted_spec.userdata = this;

    // 打开音频设备
    while (mAudioDevice->open(&wanted_spec, &spec) < 0) {
        LOGW("MediaPlayer->Failed to open audio device: (%d channels, %d Hz)!", wanted_spec.channels, wanted_spec.freq);
        // 打开音频设备失败的话，重新设置wanted_spec相关参数
        wanted_spec.channels = next_nb_channels[FFMIN(7, wanted_spec.channels)];
        if (!wanted_spec.channels) {
            // 重新设置采样率
            wanted_spec.freq = next_sample_rates[next_sample_rate_idx--];
            wanted_spec.channels = wanted_nb_channels;
            if (!wanted_spec.freq) {
                LOGE("MediaPlayer->No more combinations to try, audio open failed");
                return -1;
            }
        }
        wanted_channel_layout = av_get_default_channel_layout(wanted_spec.channels);
    }

    // 不支持16bit的采样精度
    if (spec.format != AV_SAMPLE_FMT_S16) {
        LOGE("MediaPlayer->audio format %d is not supported!", spec.format);
        return -1;
    }

    // 不支持所期望的采样率
    if (spec.channels != wanted_spec.channels) {
        wanted_channel_layout = av_get_default_channel_layout(spec.channels);
        if (!wanted_channel_layout) {
            LOGE("MediaPlayer->channel count %d is not supported!", spec.channels);
            return -1;
        }
    }

    // 初始化音频重采样器
    if (!mAudioResampler) {
        mAudioResampler = new AudioResampler(mPlayerState, mAudioDecoder, mMediaSync);
    }
    // 设置需要重采样的参数
    mAudioResampler->setResampleParams(&spec, wanted_channel_layout);

    return spec.size;
}

/**
 * 取PCM数据
 * @param stream
 * @param len
 */
void MediaPlayer::pcmQueueCallback(uint8_t *stream, int len) {
    if (!mAudioResampler) {
        memset(stream, 0, sizeof(len));
        return;
    }
    mAudioResampler->pcmQueueCallback(stream, len);
    if (mPlayerState->message_queue && mPlayerState->sync_type != AV_SYNC_VIDEO) {
        mPlayerState->message_queue->postMessage(MSG_CURRENT_POSITION, getCurrentPosition(), mPlayerState->video_duration);
    }
}

int MediaPlayer::startRecord(const char *filePath) {
    LOGD("MediaPlayer->start record --- file path=[%s]", filePath);
    mVideoRecorder = new VideoRecorder(mFormatCtx, &filePath);
//
//    int ret = avformat_alloc_output_context2(&mOutputCtx, NULL, "mpegts", filePath);
//    if (ret < 0) {
//        LOGE("MediaPlayer->VideoRecorder->Allocate an AVFormatContext for an output format failed,err=%d", ret);
//        closeOutput();
//        return false;
//    }
//
////    ret = avio_open(&mOutputCtx->pb,filePath,AVIO_FLAG_READ_WRITE);
//    ret = avio_open2(&mOutputCtx->pb, filePath, AVIO_FLAG_WRITE, NULL, NULL);
//    if (ret < 0) {
//        LOGE("MediaPlayer->VideoRecorder->Create and initialize a AVIOContext failed,err=%d", ret);
//        closeOutput();
//        return false;
//    }
//
//    int nb_streams = mFormatCtx->nb_streams;
//    for (int i = 0; i < nb_streams; i++) {
//        AVStream *stream = avformat_new_stream(mOutputCtx, mFormatCtx->streams[i]->codec->codec);
//        ret = avcodec_copy_context(stream->codec, mFormatCtx->streams[i]->codec);
//        if (ret < 0) {
//            LOGE("MediaPlayer->VideoRecorder->copy coddec context failed");
//        }
//    }
//
//    ret = avformat_write_header(mOutputCtx, NULL);
//    if (ret < 0) {
//        LOGE("MediaPlayer->VideoRecorder->write the stream header to an output media file failed,err=%d", ret);
//        closeOutput();
//        return false;
//    }
    return mVideoRecorder->start();
}

int MediaPlayer::stopRecord() {
//    av_write_trailer(mOutputCtx);
//    closeOutput();
    if (mVideoRecorder != NULL) {
        return mVideoRecorder->stop();
    } else {
        return true;
    }
}

//void MediaPlayer::closeOutput() {
//    if (mOutputCtx) {
//        for (int i = 0; i < mOutputCtx->nb_streams; i++) {
//            AVStream *avStream = mOutputCtx->streams[i];
//            if (avStream) {
//                AVCodecContext *codecContext = avStream->codec;
//                avcodec_close(codecContext);
//            }
//        }
//        avformat_close_input(&mOutputCtx);
//        avformat_free_context(mOutputCtx);
//    }
//}
//
int MediaPlayer::isRecording() {
    if (mVideoRecorder != NULL) {
        return mVideoRecorder->isRecording;
    } else {
        return false;
    }
}

int MediaPlayer::screenshot(const char *filePath) {
    return false;
}