#ifndef FFMPEG4_VIDEODECODER_H
#define FFMPEG4_VIDEODECODER_H

#include "MediaDecoder.h"
#include "PlayerState.h"
#include "MediaClock.h"

/**
 * 视频解码器
 */
class VideoDecoder : public MediaDecoder {
public:
    /**
     * @param pFormatCtx
     * @param avctx
     * @param stream
     * @param streamIndex
     * @param playerState
     */
    VideoDecoder(AVFormatContext *pFormatCtx,
                 AVCodecContext *avctx,
                 AVStream *stream,
                 int streamIndex,
                 PlayerState *playerState);

    /**
     */
    virtual ~VideoDecoder();

    /**
     * @param masterClock
     */
    void setMasterClock(MediaClock *masterClock);

    /**
     */
    void start() override;// override 保留字表示当前函数重写了基类的虚函数

    /**
     */
    void stop() override;

    /**
     */
    void flush() override;

    /**
     * @return
     */
    int getFrameSize();

    /**
     * @return
     */
    int getRotate();

    /**
     * @return
     */
    FrameQueue *getFrameQueue();

    /**
     * @return
     */
    AVFormatContext *getFormatContext();

    /**
     */
    void run() override;

private:
    /**
     * 解码视频帧
     * @return
     */
    int decodeVideo();

private:
    AVFormatContext *pFormatCtx;    // 解复用上下文
    FrameQueue *mFrameQueue;        // 帧队列
    int mRotate;                    // 旋转角度
    bool mExit;                     // 退出标志
    Thread *mDecodeThread;          // 解码线程
    MediaClock *mMasterClock;       // 主时钟
};

#endif //FFMPEG4_VIDEODECODER_H
