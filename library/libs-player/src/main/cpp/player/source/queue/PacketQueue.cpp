#include "PacketQueue.h"

PacketQueue::PacketQueue() {
    mAbortRequest = 0;
    mFirstPKT = NULL;
    mLastPKT = NULL;
    mNB_packets = 0;
    mSize = 0;
    mDuration = 0;
}

PacketQueue::~PacketQueue() {
    abort();
    flush();
}

/**
 * 入队数据包
 * @param pkt
 * @return
 */
int PacketQueue::put(AVPacket *pkt) {
    // 一个队列元素，包括当前pkt和下一个pkt的指针
    PacketList *pkt1;

    if (mAbortRequest) {
        return -1;
    }
    // 分配结构体内存
    pkt1 = (PacketList *) av_malloc(sizeof(PacketList));
    if (!pkt1) {
        return -1;
    }
    // 赋值
    pkt1->pkt = *pkt;
    pkt1->next = NULL;

    if (!mLastPKT) { // 如果最后一个元素为null，代表当前是添加第一个pkt
        mFirstPKT = pkt1;
    } else {
        // 连接到上一个元素的后面
        mLastPKT->next = pkt1; //最后元素的下一个元素的指针指向pkt1
    }
    // 最后一个元素赋值为pkt1
    mLastPKT = pkt1;
    mNB_packets++; // 数据大小
    mSize += pkt1->pkt.size + sizeof(*pkt1);
    mDuration += pkt1->pkt.duration;
    return 0;
}

/**
 * 入队数据包
 * @param pkt
 * @return
 */
int PacketQueue::pushPacket(AVPacket *pkt) {
    int ret;
    // 线程安全
    mMutex.lock();
    ret = put(pkt);
    // 通知阻塞的线程，比如取数据线程被阻塞的时候，这里可以通知它有数据可以取了，解除阻塞
    mCondition.signal();
    mMutex.unlock();

    if (ret < 0) {
        av_packet_unref(pkt);
    }

    return ret;
}

int PacketQueue::pushNullPacket(int stream_index) {
    AVPacket pkt1, *pkt = &pkt1;
    av_init_packet(pkt);
    pkt->data = NULL;
    pkt->size = 0;
    pkt->stream_index = stream_index;
    return pushPacket(pkt);
}

/**
 * 清空队列
 */
void PacketQueue::flush() {
    PacketList *pkt, *pkt1;

    mMutex.lock();
    // pkt = mFirstPKT是最开始初始化条件，判断pkt是否为真，pkt = pkt1是执行括号后更新的条件
    for (pkt = mFirstPKT; pkt; pkt = pkt1) {
        pkt1 = pkt->next;
        av_packet_unref(&pkt->pkt);
        av_freep(&pkt);
    }
    mLastPKT = NULL;
    mFirstPKT = NULL;
    mNB_packets = 0;
    mSize = 0;
    mDuration = 0;
    mCondition.signal();
    mMutex.unlock();
}

/**
 * 队列终止
 */
void PacketQueue::abort() {
    mMutex.lock();
    mAbortRequest = 1;
    mCondition.signal();
    mMutex.unlock();
}

/**
 * 队列开始
 */
void PacketQueue::start() {
    mMutex.lock();
    mAbortRequest = 0;
    mCondition.signal();
    mMutex.unlock();
}

/**
 * 取出数据包
 * @param pkt
 * @return
 */
int PacketQueue::getPacket(AVPacket *pkt) {
    return getPacket(pkt, 1);
}

/**
 * 取出数据包
 * @param pkt
 * @param block 是否阻塞
 * @return
 */
int PacketQueue::getPacket(AVPacket *pkt, int block) {
    PacketList *pkt1;
    int ret;

    mMutex.lock();

    for (;;) {
        //如果禁止取数据，则break
        if (mAbortRequest) {
            ret = -1;
            break;
        }
        // 取最前面的那个元素
        pkt1 = mFirstPKT;
        if (pkt1) { // 首元素是否为null，不为null代表队列有元素
            mFirstPKT = pkt1->next; // 取出mFirstPKT，所以首元素变为下一个
            if (!mFirstPKT) { // 没有元素了
                mLastPKT = NULL;
            }
            mNB_packets--;
            mSize -= pkt1->pkt.size + sizeof(*pkt1);
            mDuration -= pkt1->pkt.duration;
            *pkt = pkt1->pkt;
            av_free(pkt1); // 释放
            ret = 1;
            break;
        } else if (!block) { // 不阻塞
            ret = 0;
            break;
        } else { // 阻塞
            mCondition.wait(mMutex);
        }
    }
    mMutex.unlock();
    return ret;
}

int PacketQueue::getPacketSize() {
    Mutex::Autolock lock(mMutex);
    return mNB_packets;
}

int PacketQueue::getSize() {
    return mSize;
}

int64_t PacketQueue::getDuration() {
    return mDuration;
}

int PacketQueue::isAbort() {
    return mAbortRequest;
}
