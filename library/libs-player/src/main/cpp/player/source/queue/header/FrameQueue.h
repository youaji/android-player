#ifndef FFMPEG4_FRAMEQUEUE_H
#define FFMPEG4_FRAMEQUEUE_H

#include "Mutex.h"
#include "Condition.h"
#include "FFmpegUtils.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

#define FRAME_QUEUE_SIZE 10

/**
 */
typedef struct Frame {
    AVFrame *frame;     //
    AVSubtitle sub;     //
    double pts;         // presentation timestamp for the frame
    double duration;    // estimated duration of the frame
    int width;          //
    int height;         //
    int format;         //
    int uploaded;       //
} Frame;

/**
 */
class FrameQueue {

public:
    /**
     * @param max_size
     * @param keep_last
     */
    FrameQueue(int max_size, int keep_last);

    /**
     */
    virtual ~FrameQueue();

    /**
     */
    void start();

    /**
     */
    void abort();

    /**
     * @return
     */
    Frame *currentFrame();

    /**
     * @return
     */
    Frame *nextFrame();

    /**
     * @return
     */
    Frame *lastFrame();

    /**
     * 取出frame数组中的可写入元素的指针，取出来以后给元素赋值
     * @return frame
     */
    Frame *peekWritable();

    /**
     * 在调用了peekWritable以后，会写入元素到数组中，写入成功以后调用这个方法，代表插入成功，然后改变一些数据值
     */
    void pushFrame();

    /**
     * 代表放弃一个frame
     */
    void popFrame();

    /**
     */
    void flush();

    /**
     * @return
     */
    int getFrameSize();

    /**
     * @return
     */
    int getShowIndex() const;

private:
    /**
     * @param vp
     */
    void unrefFrame(Frame *vp);

private:
    Mutex mMutex;                   //
    Condition mCondition;           //
    int mAbortRequest;              //
    Frame mQueue[FRAME_QUEUE_SIZE]; //
    int mR_index;                   //
    int mW_index;                   //
    int mSize;                      //
    int mMaxSize;                   //
    int mKeepLast;                  // 表示是否保持最后一个元素
    int mShowIndex;                 //
};

#endif //FFMPEG4_FRAMEQUEUE_H
