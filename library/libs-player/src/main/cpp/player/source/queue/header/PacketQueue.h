#ifndef FFMPEG4_PACKETQUEUE_H
#define FFMPEG4_PACKETQUEUE_H

#include <queue>
#include "Mutex.h"
#include "Condition.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

/**
 * 此结构体包含一个pkt和下一个pkt的指针
 */
typedef struct PacketList {
    AVPacket pkt;
    struct PacketList *next;
} PacketList;

/**
 * 备注：这里不用std::queue是为了方便计算队列占用内存和队列的时长，在解码的时候要用到
 */
class PacketQueue {
public:
    /**
     */
    PacketQueue();

    /**
     */
    virtual ~PacketQueue();

    /**
     * 入队数据包
     */
    int pushPacket(AVPacket *pkt);

    /**
     * 入队空数据包
     * @param stream_index
     * @return
     */
    int pushNullPacket(int stream_index);

    /**
     * 刷新
     */
    void flush();

    /**
     * 终止
     */
    void abort();

    /**
     * 开始
     */
    void start();

    /**
     * 获取数据包
     * @param pkt
     * @return
     */
    int getPacket(AVPacket *pkt);

    /**
     * 获取数据包
     * @param pkt
     * @param block
     * @return
     */
    int getPacket(AVPacket *pkt, int block);

    /**
     * @return
     */
    int getPacketSize();

    /**
     * @return
     */
    int getSize();

    /**
     * @return
     */
    int64_t getDuration();

    /**
     * @return
     */
    int isAbort();

private:
    /**
     * @param pkt
     * @return
     */
    int put(AVPacket *pkt);

private:
    Mutex mMutex;                       //
    Condition mCondition;               //
    PacketList *mFirstPKT, *mLastPKT;   //
    int mNB_packets;                    //
    int mSize;                          //
    int64_t mDuration;                  //
    int mAbortRequest;                  //
};

#endif //FFMPEG4_PACKETQUEUE_H
