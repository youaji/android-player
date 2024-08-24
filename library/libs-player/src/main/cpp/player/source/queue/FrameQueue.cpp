#include "FrameQueue.h"

FrameQueue::FrameQueue(int max_size, int keep_last) {
    memset(mQueue, 0, sizeof(Frame) * FRAME_QUEUE_SIZE);
    this->mMaxSize = FFMIN(max_size, FRAME_QUEUE_SIZE);
    this->mKeepLast = (keep_last != 0);
    // 为数组元素frame分配空间
    for (int i = 0; i < this->mMaxSize; ++i) {
        mQueue[i].frame = av_frame_alloc();
    }
    mAbortRequest = 1;
    mR_index = 0;
    mW_index = 0;
    mSize = 0;
    mShowIndex = 0;
}

FrameQueue::~FrameQueue() {
    for (int i = 0; i < mMaxSize; ++i) {
        Frame *vp = &mQueue[i];
        unrefFrame(vp);
        av_frame_free(&vp->frame);
    }
}

void FrameQueue::start() {
    mMutex.lock();
    mAbortRequest = 0;
    mCondition.signal();
    mMutex.unlock();
}

void FrameQueue::abort() {
    mMutex.lock();
    mAbortRequest = 1;
    mCondition.signal();
    mMutex.unlock();
}

Frame *FrameQueue::currentFrame() {
    return &mQueue[(mR_index + mShowIndex) % mMaxSize];
}

Frame *FrameQueue::nextFrame() {
    return &mQueue[(mR_index + mShowIndex + 1) % mMaxSize];
}

Frame *FrameQueue::lastFrame() {
    return &mQueue[mR_index];
}

Frame *FrameQueue::peekWritable() {
    mMutex.lock();
    // 如果当前队列的大小已经超过了允许最大的值mMaxSize,那么就先等待
    while (mSize >= mMaxSize && !mAbortRequest) {
        mCondition.wait(mMutex);
    }
    mMutex.unlock();

    if (mAbortRequest) {
        return NULL;
    }

    return &mQueue[mW_index];
}

void FrameQueue::pushFrame() {
    if (++mW_index == mMaxSize) {
        mW_index = 0;
    }
    mMutex.lock();
    mSize++;
    mCondition.signal();
    mMutex.unlock();
}

void FrameQueue::popFrame() {
    // mKeepLast表示是否保持最后一个元素
    if (mKeepLast && !mShowIndex) {
        mShowIndex = 1;
        return;
    }
    unrefFrame(&mQueue[mR_index]);
    if (++mR_index == mMaxSize) {
        mR_index = 0;
    }
    mMutex.lock();
    mSize--;
    mCondition.signal();
    mMutex.unlock();
}

void FrameQueue::flush() {
    while (getFrameSize() > 0) {
        popFrame();
    }
}

int FrameQueue::getFrameSize() {
    return mSize - mShowIndex;
}

void FrameQueue::unrefFrame(Frame *vp) {
    av_frame_unref(vp->frame);
    avsubtitle_free(&vp->sub);
}

int FrameQueue::getShowIndex() const {
    return mShowIndex;
}