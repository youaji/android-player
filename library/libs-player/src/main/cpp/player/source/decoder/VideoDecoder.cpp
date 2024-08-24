#include "VideoDecoder.h"

VideoDecoder::VideoDecoder(AVFormatContext *pFormatCtx,
                           AVCodecContext *avctx,
                           AVStream *stream,
                           int streamIndex,
                           PlayerState *playerState) : MediaDecoder(avctx, stream, streamIndex, playerState) {

    this->pFormatCtx = pFormatCtx;
    mFrameQueue = new FrameQueue(VIDEO_QUEUE_SIZE, 1);
    mExit = true;
    mDecodeThread = NULL;
    mMasterClock = NULL;
    // 旋转角度
    AVDictionaryEntry *entry = av_dict_get(stream->metadata, "rotate", NULL, AV_DICT_MATCH_CASE);
    if (entry && entry->value) {
        mRotate = atoi(entry->value);
    } else {
        mRotate = 0;
    }
}

VideoDecoder::~VideoDecoder() {
    mMutex.lock();
    pFormatCtx = NULL;
    if (mFrameQueue) {
        mFrameQueue->flush();
        delete mFrameQueue;
        mFrameQueue = NULL;
    }
    mMasterClock = NULL;
    mMutex.unlock();
}

void VideoDecoder::setMasterClock(MediaClock *masterClock) {
    Mutex::Autolock lock(mMutex);
    this->mMasterClock = masterClock;
}

// 开始视频的解码
void VideoDecoder::start() {
    MediaDecoder::start();

    if (mFrameQueue) {
        mFrameQueue->start();
    }

    // 解码线程
    if (!mDecodeThread) {
        LOGD("VideoDecoder->开启视频解码线程");
        mDecodeThread = new Thread(this);
        mDecodeThread->start();
        mExit = false;
    }
}

void VideoDecoder::stop() {
    MediaDecoder::stop();

    if (mFrameQueue) {
        mFrameQueue->abort();
    }

    mMutex.lock();
    while (!mExit) {
        mCondition.wait(mMutex);
    }
    mMutex.unlock();

    if (mDecodeThread) {
        mDecodeThread->join();
        delete mDecodeThread;
        mDecodeThread = NULL;
        LOGD("VideoDecoder->删除视频解码线程");
    }
}

void VideoDecoder::flush() {
    mMutex.lock();
    MediaDecoder::flush();
    if (mFrameQueue) {
        mFrameQueue->flush();
    }
    mCondition.signal();
    mMutex.unlock();
}

int VideoDecoder::getFrameSize() {
    Mutex::Autolock lock(mMutex);
    return mFrameQueue ? mFrameQueue->getFrameSize() : 0;
}

int VideoDecoder::getRotate() {
    Mutex::Autolock lock(mMutex);
    return mRotate;
}

FrameQueue *VideoDecoder::getFrameQueue() {
    Mutex::Autolock lock(mMutex);
    return mFrameQueue;
}

AVFormatContext *VideoDecoder::getFormatContext() {
    Mutex::Autolock lock(mMutex);
    return pFormatCtx;
}

void VideoDecoder::run() {
    decodeVideo();
}

/**
 * 解码视频数据包并放入帧队列
 * @return
 */
int VideoDecoder::decodeVideo() {
    AVFrame *frame = av_frame_alloc();
    Frame *vp;
    int got_picture;
    int ret;

    AVRational tb = mAVStream->time_base;
    AVRational frame_rate = av_guess_frame_rate(pFormatCtx, mAVStream, NULL);

    if (!frame) {
        mExit = true;
        mCondition.signal();
        return AVERROR(ENOMEM);
    }

    // 分配未解码数据的内存
    AVPacket *packet = av_packet_alloc();
    if (!packet) {
        mExit = true;
        mCondition.signal();
        return AVERROR(ENOMEM);
    }

    // 循环从队列中取出未解码的数据，解码成功后将数据结果存入到另一个队列中
    for (;;) {

        if (mAbortRequest || mPlayerState->abort_request) {
            ret = -1;
            break;
        }

        if (mPlayerState->seek_request) {
            continue;
        }

        /* 取数据，如果没有数据会阻塞，这是一个生产者消费者模式 */
        if (mPacketQueue->getPacket(packet) < 0) { // 可能会被阻塞
            ret = -1;
            break;
        }

        mPlayerState->mutex.lock();
        // 送去解码
        ret = avcodec_send_packet(mAVCodecCtx, packet);
        if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
            av_packet_unref(packet);
            mPlayerState->mutex.unlock();
            continue;
        }
        // 得到解码帧
        ret = avcodec_receive_frame(mAVCodecCtx, frame);
        mPlayerState->mutex.unlock();

        if (ret < 0 && ret != AVERROR_EOF) {
            av_frame_unref(frame);
            av_packet_unref(packet);
            continue;
        } else { // 解码正常
            got_picture = 1;
            // 是否重排pts，默认情况下需要重排pts的
            if (mPlayerState->reorder_video_pts == 1) {
                frame->pts = av_frame_get_best_effort_timestamp(frame);
            } else if (!mPlayerState->reorder_video_pts) {
                frame->pts = frame->pkt_dts;
            }

            // 丢帧处理
            if (mMasterClock != NULL) {
                double dpts = NAN;

                if (frame->pts != AV_NOPTS_VALUE) {
                    dpts = av_q2d(mAVStream->time_base) * frame->pts;
                }
                // 计算帧的长宽比
                frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(pFormatCtx, mAVStream, frame);
                // 是否需要做舍帧操作
                // 主要看音视频同步是否差距过大
                if (mPlayerState->frame_drop > 0 ||
                    (mPlayerState->frame_drop > 0 && mPlayerState->sync_type != AV_SYNC_VIDEO)) {
                    if (frame->pts != AV_NOPTS_VALUE) {
                        double diff = dpts - mMasterClock->getClock();
                        if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD && diff < 0 &&
                            mPacketQueue->getPacketSize() > 0) {
                            av_frame_unref(frame);
                            got_picture = 0;
                        }
                    }
                }
            }
        }

        if (got_picture) { //解码正常
            // 取出 frame 数组中的可写入元素指针
            // 当 frame 数组满时，会阻塞等待
            if (!(vp = mFrameQueue->peekWritable())) { // 可能会被阻塞
                ret = -1;
                break;
            }

            // 复制参数
            vp->uploaded = 0;
            vp->width = frame->width;
            vp->height = frame->height;
            vp->format = frame->format;
            vp->pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
            // 计算一帧的时长
            vp->duration = frame_rate.num && frame_rate.den ? av_q2d((AVRational) {frame_rate.den, frame_rate.num}) : 0;
            av_frame_move_ref(vp->frame, frame); //移动引用的意思
            // 写入数据成功
            // 这是一个生产者消费者模式的队列
            mFrameQueue->pushFrame();
        }

        // 释放数据包和缓冲帧的引用
        // 防止内存泄漏
        av_frame_unref(frame);
        av_packet_unref(packet);
    }

    av_frame_free(&frame);
    av_free(frame);
    frame = NULL;

    // 本身指针 packet 是操作了一个内存空间的
    // 用完之后要释放，否则内存泄漏
    av_packet_free(&packet);
    av_free(packet);
    packet = NULL;

    // 等待线程结束后再删除线程对象
    mExit = true;
    mCondition.signal();

    return ret;
}
