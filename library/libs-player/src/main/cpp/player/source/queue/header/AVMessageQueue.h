#ifndef FFMPEG4_AVMESSAGEQUEUE_H
#define FFMPEG4_AVMESSAGEQUEUE_H

#include "Mutex.h"
#include "Condition.h"
#include <cstring>
#include <assert.h>

extern "C" {
#include "libavutil/mem.h"
}

#include "PlayerMessage.h"

/**
 */
typedef struct AVMessage {
    int what;
    int arg1;
    int arg2;
    void *obj;

    void (*free)(void *obj);

    struct AVMessage *next;
} AVMessage;

/**
 */
inline static void message_init(AVMessage *msg) {
    memset(msg, 0, sizeof(AVMessage));
}

/**
 */
inline static void message_free(void *obj) {
    av_free(obj);
}

/**
 */
inline static void message_free_resouce(AVMessage *msg) {
    if (!msg || !msg->obj) {
        return;
    }
    assert(msg->free);
    msg->free(msg->obj);
    msg->obj = NULL;
}

/**
 */
class AVMessageQueue {
public:
    /**
     */
    AVMessageQueue();

    virtual ~AVMessageQueue();

    /**
     */
    void start();

    /**
     */
    void stop();

    /**
     */
    void flush();

    /**
     */
    void release();

    /**
     * @param what
     */
    void postMessage(int what);

    /**
     * @param what
     * @param arg1
     */
    void postMessage(int what, int arg1);

    /**
     * @param what
     * @param arg1
     * @param arg2
     */
    void postMessage(int what, int arg1, int arg2);

    /**
     * @param what
     * @param arg1
     * @param arg2
     * @param obj
     * @param len
     */
    void postMessage(int what, int arg1, int arg2, void *obj, int len);

    /**
     * @param msg
     * @return
     */
    int getMessage(AVMessage *msg);

    /**
     * @param msg
     * @param block
     * @return
     */
    int getMessage(AVMessage *msg, int block);

    /**
     * @param what
     */
    void removeMessage(int what);

private:
    /**
     * @param msg
     * @return
     */
    int putMessage(AVMessage *msg);

private:
    Mutex mMutex;                       //
    Condition mCondition;               //
    AVMessage *mFirstMsg, *mLastMsg;    //
    bool mAbortRequest;                 //
    int mSize;                          //
};

#endif //FFMPEG4_AVMESSAGEQUEUE_H
