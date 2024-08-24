#include "MediaDecoder.h"

MediaDecoder::MediaDecoder(AVCodecContext *avctx,
                           AVStream *stream,
                           int streamIndex,
                           PlayerState *playerState) {
    mPacketQueue = new PacketQueue();
    this->mAVCodecCtx = avctx;
    this->mAVStream = stream;
    this->mStreamIndex = streamIndex;
    this->mPlayerState = playerState;
}

MediaDecoder::~MediaDecoder() {
    mMutex.lock();
    if (mPacketQueue) {
        mPacketQueue->flush();
        delete mPacketQueue;
        mPacketQueue = NULL;
    }
    if (mAVCodecCtx) {
        avcodec_close(mAVCodecCtx);
        avcodec_free_context(&mAVCodecCtx);
        mAVCodecCtx = NULL;
    }
    mPlayerState = NULL;
    mMutex.unlock();
}

void MediaDecoder::start() {
    if (mPacketQueue) {
        mPacketQueue->start();
    }
    mMutex.lock();
    mAbortRequest = false;
    mCondition.signal();
    mMutex.unlock();
}

void MediaDecoder::stop() {
    mMutex.lock();
    mAbortRequest = true;
    mCondition.signal();
    mMutex.unlock();
    if (mPacketQueue) {
        mPacketQueue->abort();
    }
}

void MediaDecoder::flush() {
    if (mPacketQueue) {
        mPacketQueue->flush();
    }
    // 定位时，音视频均需要清空缓冲区
    mPlayerState->mutex.lock();
    avcodec_flush_buffers(getCodecContext());
    mPlayerState->mutex.unlock();
}

int MediaDecoder::pushPacket(AVPacket *pkt) {
    if (mPacketQueue) {
        return mPacketQueue->pushPacket(pkt);
    }
    return 0;
}

int MediaDecoder::getPacketSize() {
    return mPacketQueue ? mPacketQueue->getPacketSize() : 0;
}

int MediaDecoder::getStreamIndex() {
    return mStreamIndex;
}

AVStream *MediaDecoder::getStream() {
    return mAVStream;
}

AVCodecContext *MediaDecoder::getCodecContext() {
    return mAVCodecCtx;
}

int MediaDecoder::getMemorySize() {
    return mPacketQueue ? mPacketQueue->getSize() : 0;
}

int MediaDecoder::hasEnoughPackets() {
    Mutex::Autolock lock(mMutex);
    return (mPacketQueue == NULL) ||
           (mPacketQueue->isAbort()) ||
           (mAVStream->disposition & AV_DISPOSITION_ATTACHED_PIC) ||
           (mPacketQueue->getPacketSize() > MIN_FRAMES) &&
           (!mPacketQueue->getDuration() || av_q2d(mAVStream->time_base) * mPacketQueue->getDuration() > 1.0);
}

void MediaDecoder::run() {
    // do nothing
}

