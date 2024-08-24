#include "MediaSync.h"

MediaSync::MediaSync(PlayerState *playerState) {
    this->mPlayerState = playerState;
    mAudioDecoder = NULL;
    mVideoDecoder = NULL;
    mAudioClock = new MediaClock();
    mVideoClock = new MediaClock();
    mExtClock = new MediaClock();

    mIsExit = true;
    mIsAbortRequest = true;
    mSyncThread = NULL;

    mForceRefresh = 0;
    mMaxFrameDuration = 10.0;
    mFrameTimerRefresh = 1;
    mFrameTimer = 0;

    mVideoDevice = NULL;
    swsContext = NULL;
    mBuffer = NULL;
    pFrameARGB = NULL;
}

MediaSync::~MediaSync() {}

void MediaSync::reset() {
    stop();
    mPlayerState = NULL;
    mVideoDecoder = NULL;
    mAudioDecoder = NULL;
    mVideoDevice = NULL;

    if (pFrameARGB) {
        av_frame_free(&pFrameARGB);
        av_free(pFrameARGB);
        pFrameARGB = NULL;
    }
    if (mBuffer) {
        av_freep(&mBuffer);
        mBuffer = NULL;
    }
    if (swsContext) {
        sws_freeContext(swsContext);
        swsContext = NULL;
    }
}

void MediaSync::start(VideoDecoder *video_decoder, AudioDecoder *audio_decoder) {
    mMutex.lock();
    this->mVideoDecoder = video_decoder;
    this->mAudioDecoder = audio_decoder;
    mIsAbortRequest = false;
    mIsExit = false;
    mCondition.signal();
    mMutex.unlock();
    if (video_decoder && !mSyncThread) {
        mSyncThread = new Thread(this);
        mSyncThread->start();
        LOGD("MediaSync->开启同步线程");
    }
}

void MediaSync::stop() {
    mMutex.lock();
    mIsAbortRequest = true;
    mCondition.signal();
    mMutex.unlock();

    mMutex.lock();
    while (!mIsExit) {
        mCondition.wait(mMutex);
    }
    mMutex.unlock();
    if (mSyncThread) {
        mSyncThread->join();
        delete mSyncThread;
        mSyncThread = NULL;
        LOGD("MediaSync->删除同步线程");
    }
}

void MediaSync::setVideoDevice(VideoDevice *device) {
    Mutex::Autolock lock(mMutex);
    this->mVideoDevice = device;
}

void MediaSync::setMaxDuration(double maxDuration) {
    this->mMaxFrameDuration = maxDuration;
}

void MediaSync::refreshVideoTimer() {
    mMutex.lock();
    this->mFrameTimerRefresh = 1;
    mCondition.signal();
    mMutex.unlock();
}

void MediaSync::updateAudioClock(double pts, double time) {
    mAudioClock->setClock(pts, time);
    mExtClock->syncToSlave(mAudioClock);
}

double MediaSync::getAudioDiffClock() {
    return mAudioClock->getClock() - getMasterClock();
}

void MediaSync::updateExternalClock(double pts) {
    mExtClock->setClock(pts);
}

double MediaSync::getMasterClock() {
    double val = 0;
    switch (mPlayerState->sync_type) {
        case AV_SYNC_VIDEO: {
            val = mVideoClock->getClock();
            break;
        }
        case AV_SYNC_AUDIO: {
            val = mAudioClock->getClock();
            break;
        }
        case AV_SYNC_EXTERNAL: {
            val = mExtClock->getClock();
            break;
        }
    }
    return val;
}

MediaClock *MediaSync::getAudioClock() {
    return mAudioClock;
}

MediaClock *MediaSync::getVideoClock() {
    return mVideoClock;
}

MediaClock *MediaSync::getExternalClock() {
    return mExtClock;
}

void MediaSync::run() {
    double remaining_time = 0.0;

    while (true) {
        if (mIsAbortRequest || mPlayerState->abort_request) { //停止
            if (mVideoDevice != NULL) {
                LOGE("MediaSync->mIsAbortRequest%d mAbortRequest%d", mIsAbortRequest, mPlayerState->abort_request);
                mVideoDevice->terminate();
            }
            break;
        }

        if (remaining_time > 0.0) {
            av_usleep((int64_t) (remaining_time * 1000000.0));
        }
        remaining_time = REFRESH_RATE; //刷新率

        // 暂停的时候会停留在这里
        if (!mPlayerState->pause_request || mForceRefresh) {
            refreshVideo(&remaining_time);
        }
    }

    mIsExit = true;
    mCondition.signal();
}

void MediaSync::refreshVideo(double *remaining_time) {
    double time;

    // 检查外部时钟
    if (!mPlayerState->pause_request &&
        mPlayerState->real_time &&
        mPlayerState->sync_type == AV_SYNC_EXTERNAL) {
        checkExternalClockSpeed();
    }

    for (;;) {

        if (mPlayerState->abort_request || !mVideoDecoder) {
            break;
        }

        // 判断帧队列是否存在数据
        if (mVideoDecoder->getFrameSize() > 0) {
            double lastDuration, duration, delay;
            Frame *currentFrame, *lastFrame;
            // 上一帧，正在播放的视频帧
            lastFrame = mVideoDecoder->getFrameQueue()->lastFrame();
            // 当前帧，即将要播放的视频帧
            currentFrame = mVideoDecoder->getFrameQueue()->currentFrame();
            // 判断是否需要强制更新帧的时间
            if (mFrameTimerRefresh) {
                mFrameTimer = av_gettime_relative() / 1000000.0;
                mFrameTimerRefresh = 0;
            }

            // 如果处于暂停状态，跳出循环，会一直播放正在显示的那一帧
            if (mPlayerState->abort_request || mPlayerState->pause_request) {
                break;
            }

            // 计算上一帧时长，即当前正在播放帧的正常播放时长
            lastDuration = calculateDuration(lastFrame, currentFrame);
            // 关键部位，计算延时，即到播放下一帧需要延时多长时间
            delay = calculateDelay(lastDuration);
            // 处理延时超过阈值的情况
            if (fabs(delay) > AV_SYNC_THRESHOLD_MAX) {
                if (delay > 0) {
                    delay = AV_SYNC_THRESHOLD_MAX;
                } else {
                    delay = 0;
                }
            }
            // 获取当前时间
            time = av_gettime_relative() / 1000000.0;
            if (isnan(mFrameTimer) || time < mFrameTimer) {
                mFrameTimer = time;
            }
            // 如果当前时间小于当前帧的开始播放时间，表示播放时机未到
            if (time < mFrameTimer + delay) {
                //取一个值作为remaining_time返回给上一级函数调用，作为一个延时
                *remaining_time = FFMIN(mFrameTimer + delay - time, *remaining_time);
                break;
            }
            // 当前帧播放时刻到了，需要更新帧计时器，此时的帧计时器其实代表当前帧的播放时刻
            mFrameTimer += delay;
            // 帧计时器落后当前时间超过了阈值，则用当前的时间作为帧计时器时间，一般播放视频暂停以后，因为time一直在增加，而frameTimer
            // 却一直没有增加，所以time - frameTimer会大于最大阈值，此时应该将frameTimer更新为当前时间
            if (delay > 0 && time - mFrameTimer > AV_SYNC_THRESHOLD_MAX) {
                // 一般暂停后重新播放会执行这里
                mFrameTimer = time;
            }

            // 更新视频时钟，代表当前视频的播放时刻
            mMutex.lock();
            if (!isnan(currentFrame->pts)) {
                // 设置视频时钟
                mVideoClock->setClock(currentFrame->pts);
                mExtClock->syncToSlave(mVideoClock);
            }
            mMutex.unlock();

            // 如果队列中还有数据，需要拿到下一帧，然后计算间隔，并判断是否需要进行舍帧操作
            if (mVideoDecoder->getFrameSize() > 1) {
                Frame *nextFrame = mVideoDecoder->getFrameQueue()->nextFrame();
                duration = calculateDuration(currentFrame, nextFrame);
                // 条件1：可跳帧并且不是同步到视频
                // 条件2：当前帧未能及时播放，即播放完当前帧的时刻也小于当前系统时刻(time)，表示当前帧已经错过了播放时机，那么可能要弃帧了
                // 结果： 舍弃一帧，不播放当前帧
                if ((time > mFrameTimer + duration) &&
                    (mPlayerState->frame_drop > 0 || (mPlayerState->frame_drop && mPlayerState->sync_type != AV_SYNC_VIDEO))) {
                    //舍弃上一帧，不播放当前帧，继续循环
                    mVideoDecoder->getFrameQueue()->popFrame();
                    continue;
                }
            }

            // 播放当前帧时机已到
            // 取出并舍弃一帧，即上一帧 lastFrame，
            // 此时当前帧就变成了上一帧，所以下面renderVideo方法中取出当前帧播放的时候，调用的是lastFrame
            mVideoDecoder->getFrameQueue()->popFrame();
            // 当还需要延时的时候，即当前帧播放时机未到时，是执行不到这里的，
            // 所以延时阶段forceRefresh为0，所以不会调用renderVideo方法，
            // 但是当延时到期以后，就会执行到这里，
            // 代表需要进行下一帧视频的渲染了，然后调用renderVideo方法，调用之后forceRefresh又为0
            mForceRefresh = 1;
        }

        break;
    }

    // 如果是同步到视频时钟，在这里也通知当前时长
    if (mPlayerState->message_queue && mPlayerState->sync_type == AV_SYNC_VIDEO) {
        // 起始延时
        int64_t start_time = mVideoDecoder->getFormatContext()->start_time;
        int64_t start_diff = 0;
        if (start_time > 0) {
//        if (start_time > 0 && start_time != AV_NOPTS_VALUE) {
            start_diff = av_rescale(start_time, 1000, AV_TIME_BASE);
        }
        // 计算主时钟的时间
        int64_t pos;
        double clock = getMasterClock();
        if (isnan(clock)) {
            pos = mPlayerState->seek_pos;
        } else {
            pos = (int64_t) (clock * 1000);
        }
        if (pos < 0 || pos < start_diff) {
            pos = 0;
        }
        pos = (long) (pos - start_diff);
        if (mPlayerState->video_duration < 0) {
            pos = 0;
        }
        mPlayerState->message_queue->postMessage(MSG_CURRENT_POSITION, pos, mPlayerState->video_duration);
    }

    // 渲染视频帧
    if (!mPlayerState->display_disable &&
        mForceRefresh &&
        mVideoDecoder &&
        mVideoDecoder->getFrameQueue()->getShowIndex()) {
        renderVideo(); // 渲染
    }
    mForceRefresh = 0;
}

void MediaSync::checkExternalClockSpeed() {
    if ((mVideoDecoder && mVideoDecoder->getPacketSize() <= EXTERNAL_CLOCK_MIN_FRAMES) ||
        (mAudioDecoder && mAudioDecoder->getPacketSize() <= EXTERNAL_CLOCK_MIN_FRAMES)) {

        mExtClock->setSpeed(FFMAX(EXTERNAL_CLOCK_SPEED_MIN, mExtClock->getSpeed() - EXTERNAL_CLOCK_SPEED_STEP));

    } else if ((!mVideoDecoder || mVideoDecoder->getPacketSize() > EXTERNAL_CLOCK_MAX_FRAMES) &&
               (!mAudioDecoder || mAudioDecoder->getPacketSize() > EXTERNAL_CLOCK_MAX_FRAMES)) {

        mExtClock->setSpeed(FFMIN(EXTERNAL_CLOCK_SPEED_MAX, mExtClock->getSpeed() + EXTERNAL_CLOCK_SPEED_STEP));

    } else {

        double speed = mExtClock->getSpeed();
        if (speed != 1.0) {
            mExtClock->setSpeed(speed + EXTERNAL_CLOCK_SPEED_STEP * (1.0 - speed) / fabs(1.0 - speed));
        }

    }
}

double MediaSync::calculateDelay(double delay) {
    double sync_threshold, diff;
    // 如果不是同步到视频，则需要计算延时时间
    if (mPlayerState->sync_type != AV_SYNC_VIDEO) {
        // 计算差值，mVideoClock->getClock()是当前播放帧的时间戳，这里计算当前播放视频帧的时间戳和音频时间戳的差值
        diff = mVideoClock->getClock() - getMasterClock(); // 这里是同步到音频时钟，所以主时钟就是音频时钟
        // 计算阈值
        sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
        if (!isnan(diff) && fabs(diff) < mMaxFrameDuration) {
            if (diff <= -sync_threshold) { // 视频慢了，需要加快视频播放
                delay = FFMAX(0, delay + diff);
            } else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD) { // 视频快了而且上一帧视频的播放时长超过了阈值
                delay = delay + diff;
            } else if (diff >= sync_threshold) { // 视频快了
                delay = 2 * delay;
            }
        }
    }

//    LOGD("MediaSync->media sync delay=%0.3f A-V=%f", delay, -diff);

    return delay;
}

double MediaSync::calculateDuration(Frame *vp, Frame *nextvp) {
    double duration = nextvp->pts - vp->pts;
    if (isnan(duration) || duration <= 0 || duration > mMaxFrameDuration) {
        return vp->duration;
    } else {
        return duration;
    }
}

long MediaSync::getCurrentPosition() {
    int64_t currentPosition;
    // 处于定位
    if (mPlayerState->seek_request) {
        currentPosition = mPlayerState->seek_pos;
    } else {

        // 起始延时
        int64_t start_time = mVideoDecoder->getFormatContext()->start_time;

        int64_t start_diff = 0;
        if (start_time > 0) {
//        if (start_time > 0 && start_time != AV_NOPTS_VALUE) {
            start_diff = av_rescale(start_time, 1000, AV_TIME_BASE);
        }

        // 计算主时钟的时间
        int64_t pos;
        double clock = getMasterClock();
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

void MediaSync::renderVideo() {
    mMutex.lock();
    if (!mVideoDecoder || !mVideoDevice) {
        mMutex.unlock();
        return;
    }
    // 取出帧进行播放
    Frame *vp = mVideoDecoder->getFrameQueue()->lastFrame();

    int ret;
    if (!vp->uploaded) {
        // 根据图像格式更新纹理数据
        switch (vp->frame->format) {
            // YUV420P 和 YUVJ420P 除了色彩空间不一样之外，其他的没什么区别
            // YUV420P表示的范围是 16 ~ 235，而YUVJ420P表示的范围是0 ~ 255
            // 这里做了兼容处理，后续可以优化，shader已经过验证
            case AV_PIX_FMT_YUVJ420P:
            case AV_PIX_FMT_YUV420P: {
                // LOGE("MediaSync->yuv数据");
                // 初始化纹理
                mVideoDevice->onInitTexture(
                        vp->frame->width,
                        vp->frame->height,
                        FMT_YUV420P,
                        BLEND_NONE
                );

                if (vp->frame->linesize[0] < 0 || vp->frame->linesize[1] < 0 || vp->frame->linesize[2] < 0) {
                    LOGE("MediaSync->Negative lineSize is not supported for YUV.");
                    return;
                }
                // 上载纹理数据，比如YUV420中，data[0]专门存Y，data[1]专门存U，data[2]专门存V
                ret = mVideoDevice->onUpdateYUV(
                        vp->frame->data[0], vp->frame->linesize[0],
                        vp->frame->data[1], vp->frame->linesize[1],
                        vp->frame->data[2], vp->frame->linesize[2]
                );
                if (ret < 0) {
                    return;
                }
                break;
            }

                // 直接渲染BGRA8888，即一个像素4个字节，因为c++数据存储使用的是小端模式，跟大端模式顺序相反，所以BGRA其实就是ARGB
            case AV_PIX_FMT_BGRA: {
                // LOGE("MediaSync->rgb数据");
                mVideoDevice->onInitTexture(
                        vp->frame->width,
                        vp->frame->height,
                        FMT_ARGB,
                        BLEND_NONE
                );
                ret = mVideoDevice->onUpdateARGB(vp->frame->data[0], vp->frame->linesize[0]);
                if (ret < 0) {
                    return;
                }
                break;
            }

                // 其他格式转码成BGRA格式再做渲染
            default: {

                // 首先通过传入的swsContext上下文根据参数去缓存空间里面找，如果有对应的缓存则返回，没有则开辟一个新的上下文空间
                // 参数说明：
                //    第一参数可以传NULL，默认会开辟一块新的空间。
                //    srcW,srcH, srcFormat， 原始数据的宽高和原始像素格式(YUV420)，
                //    dstW,dstH,dstFormat;   目标宽，目标高，目标的像素格式(这里的宽高可能是手机屏幕分辨率，RGBA8888)，这里不仅仅包含了尺寸的转换和像素格式的转换
                //    flag 提供了一系列的算法，快速线性，差值，矩阵，不同的算法性能也不同，快速线性算法性能相对较高。只针对尺寸的变换。对像素格式转换无此问题
                //    #define SWS_FAST_BILINEAR 1
                //    #define SWS_BILINEAR 2
                //    #define SWS_BICUBIC 4
                //    #define SWS_X      8
                //    #define SWS_POINT 0x10
                //    #define SWS_AREA 0x20
                //    #define SWS_BICUBLIN 0x40
                // 后面还有两个参数是做过滤器用的，一般用不到，传NULL，最后一个参数是跟flag算法相关，也可以传NULL。
                swsContext = sws_getCachedContext(
                        swsContext,
                        vp->frame->width, vp->frame->height,
                        (AVPixelFormat) vp->frame->format,
                        vp->frame->width, vp->frame->height,
                        AV_PIX_FMT_BGRA, SWS_BICUBIC, NULL, NULL, NULL
                );
                if (!mBuffer) {
                    // 函数的作用是通过指定像素格式、图像宽、图像高来计算所需的内存大小
                    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_BGRA, vp->frame->width, vp->frame->height, 1);
                    mBuffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
                    pFrameARGB = av_frame_alloc();
                    // 主要给pFrameARGB的保存数据的地址指针赋值，按照指定的参数指向mBuffer中对应的位置，还有linesize，比如YUV格式的数据，分别
                    // 有保存YUV三个不同数据的指针会得到空间的分配
                    av_image_fill_arrays(
                            pFrameARGB->data,
                            pFrameARGB->linesize,
                            mBuffer,
                            AV_PIX_FMT_BGRA,
                            vp->frame->width,
                            vp->frame->height,
                            1
                    );
                }
                if (swsContext != NULL) {
                    /*数据的格式转换*/
                    // 1.参数 SwsContext *c， 转换格式的上下文。也就是 sws_getContext 函数返回的结果
                    // 2.参数 const uint8_t *const srcSlice[], 输入图像的每个颜色通道的数据指针。其实就是解码后的AVFrame中的data[]数组
                    // 因为不同像素的存储格式不同，所以srcSlice[]维数也有可能不同
                    // 以YUV420P为例，它是planar格式，它的内存中的排布如下：
                    // YYYYYYYY UUUU VVVV
                    // 使用FFmpeg解码后存储在AVFrame的data[]数组中时：
                    // data[0]——-Y分量, Y1, Y2, Y3, Y4, Y5, Y6, Y7, Y8……
                    // data[1]——-U分量, U1, U2, U3, U4……
                    // data[2]——-V分量, V1, V2, V3, V4……
                    // linesize[]数组中保存的是对应通道的数据宽度
                    // linesize[0]——-Y分量的宽度
                    // linesize[1]——-U分量的宽度
                    // linesize[2]——-V分量的宽度
                    // 而RGB24，它是packed格式，它在data[]数组中则只有一维，它在存储方式如下：
                    // data[0]: R1, G1, B1, R2, G2, B2, R3, G3, B3, R4, G4, B4……
                    // 这里要特别注意，linesize[0]的值并不一定等于图片的宽度，有时候为了对齐各解码器的CPU，实际尺寸会大于图片的宽度，这点在我们编程时（比如OpengGL硬件转换/渲染）
                    // 要特别注意，否则解码出来的图像会异常
                    // 3.参数const int srcStride[]，输入图像的每个颜色通道的跨度。.也就是每个通道的行字节数，对应的是解码后的AVFrame中的linesize[]数组
                    // 根据它可以确立下一行的起始位置，不过stride和width不一定相同，这是因为：
                    // a.由于数据帧存储的对齐，有可能会向每行后面增加一些填充字节这样 stride = width + N
                    // b.packet色彩空间下，每个像素几个通道数据混合在一起，例如RGB24，每个像素3字节连续存放，因此下一行的位置需要跳过3*width字节
                    // 4.参数int srcSliceY, int srcSliceH,定义在输入图像的处理区域，srcSliceY是起始位置，srcSliceH是处理多少行。如果srcSliceY=0，srcSliceH=height，
                    // 表示一次性处理完整个图像。这种设置是为了多线程并行，例如可以创建两个线程，第一个线程处理 [0, h/2-1]行，第二个线程处理 [h/2, h-1]行。并行处理加快速度
                    // 5.参数uint8_t *const dst[], const int dstStride[]定义输出图像信息（输出的每个颜色通道数据指针，每个颜色通道行字节数）
                    sws_scale(swsContext, (uint8_t const *const *) vp->frame->data,
                              vp->frame->linesize, 0, vp->frame->height,
                              pFrameARGB->data, pFrameARGB->linesize);
                }
                mVideoDevice->onInitTexture(
                        vp->frame->width,
                        vp->frame->height,
                        FMT_ARGB,
                        BLEND_NONE,
                        mVideoDecoder->getRotate()
                );
                ret = mVideoDevice->onUpdateARGB(pFrameARGB->data[0], pFrameARGB->linesize[0]);
                if (ret < 0) {
                    return;
                }
                break;
            }
        }
        vp->uploaded = 1;
    }
    // 请求渲染视频
    if (mVideoDevice != NULL) {
        // 设置视频播放的时间戳
        mVideoDevice->setTimeStamp(isnan(vp->pts) ? 0 : vp->pts);
        mVideoDevice->onRequestRender(vp->frame->linesize[0] < 0);
    }
    // 当文件没有音频的时候，用视频时间戳来通知当前播放时间
    if (mAudioDecoder == NULL && mPlayerState->message_queue) {
        mPlayerState->message_queue->postMessage(MSG_CURRENT_POSITION, getCurrentPosition(), mPlayerState->video_duration);
    }
    mMutex.unlock();
}

