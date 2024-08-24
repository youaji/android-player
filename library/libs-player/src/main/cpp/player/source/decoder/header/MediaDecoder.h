#ifndef FFMPEG4_MEDIADECODER_H
#define FFMPEG4_MEDIADECODER_H

#include "AndroidLog.h"
#include "PlayerState.h"
#include "PacketQueue.h"
#include "FrameQueue.h"

/**
 * 媒体解码器
 */
class MediaDecoder : public Runnable {
public:
    /**
     * @param avctx
     * @param stream
     * @param streamIndex
     * @param playerState
     */
    MediaDecoder(AVCodecContext *avctx,
                 AVStream *stream,
                 int streamIndex,
                 PlayerState *playerState);

    /**
     */
    virtual ~MediaDecoder();

    /**
     */
    virtual void start();

    /**
     */
    virtual void stop();

    /**
     */
    virtual void flush();

    /**
     * @param pkt
     * @return
     */
    int pushPacket(AVPacket *pkt);

    /**
     * @return
     */
    int getPacketSize();

    /**
     * @return
     */
    int getStreamIndex();

    /**
     * @return
     */
    AVStream *getStream();

    /**
     * @return
     */
    AVCodecContext *getCodecContext();

    /**
     * @return
     */
    int getMemorySize();

    /**
     * @return
     */
    int hasEnoughPackets();

    /**
     */
    virtual void run();

protected:
    Mutex mMutex;                 //
    Condition mCondition;         //
    bool mAbortRequest;           //
    PlayerState *mPlayerState;    //
    PacketQueue *mPacketQueue;    // 数据包队列
    AVCodecContext *mAVCodecCtx;  //
    AVStream *mAVStream;          //
    int mStreamIndex;             //
};

#endif //FFMPEG4_MEDIADECODER_H
