#ifndef FFMPEG4_MEDIASYNC_H
#define FFMPEG4_MEDIASYNC_H

#include "MediaClock.h"
#include "PlayerState.h"
#include "VideoDecoder.h"
#include "AudioDecoder.h"

#include "VideoDevice.h"

/**
 * 视频媒体同步器
 */
class MediaSync : public Runnable {

public:
    /**
     * @param playerState
     */
    MediaSync(PlayerState *playerState);

    /**
     */
    virtual ~MediaSync();

    /**
     */
    void reset();

    /**
     * @param video_decoder
     * @param audio_decoder
     */
    void start(VideoDecoder *video_decoder, AudioDecoder *audio_decoder);

    /**
     */
    void stop();

    /**
     * 设置视频输出设备
     * @param device
     */
    void setVideoDevice(VideoDevice *device);

    /**
     * 设置帧最大间隔
     * @param maxDuration
     * @return
     */
    void setMaxDuration(double maxDuration);

    /**
     * 更新视频帧的计时器\n
     * 定位请求的时会强制刷新视频帧时间
     */
    void refreshVideoTimer();

    /**
     * 更新音频时钟
     * @param pts 当前播放的时间点，单位秒
     * @param time 音频回调时间，单位秒
     */
    void updateAudioClock(double pts, double time);

    /**
     * 获取音频时钟与主时钟的差值
     * @return
     */
    double getAudioDiffClock();

    /**
     * 更新外部时钟
     * @param pts
     */
    void updateExternalClock(double pts);

    /**
     * 获取主时钟\n
     * 默认以音频时钟为主时钟
     * @return 主时钟
     */
    double getMasterClock();

    /**
     */
    void run() override;

    /**
     * @return
     */
    MediaClock *getAudioClock();

    /**
     * @return
     */
    MediaClock *getVideoClock();

    /**
     * @return
     */
    MediaClock *getExternalClock();

    /**
     * @return
     */
    long getCurrentPosition();

    /**
     */
    void renderVideo();

private:
    /**
     * @param remaining_time
     */
    void refreshVideo(double *remaining_time);

    /**
     */
    void checkExternalClockSpeed();

    /**
     * @param delay 为上一帧视频的播放时长
     * @return
     */
    double calculateDelay(double delay);

    /**
     * @param vp
     * @param nextvp
     * @return
     */
    double calculateDuration(Frame *vp, Frame *nextvp);


private:
    PlayerState *mPlayerState;    // 播放器状态
    bool mIsAbortRequest;         // 停止
    bool mIsExit;                 //

    MediaClock *mAudioClock;      // 音频时钟
    MediaClock *mVideoClock;      // 视频时钟
    MediaClock *mExtClock;        // 外部时钟

    VideoDecoder *mVideoDecoder;  // 视频解码器
    AudioDecoder *mAudioDecoder;  // 视频解码器

    Mutex mMutex;
    Condition mCondition;         //
    Thread *mSyncThread;          // 同步线程

    int mForceRefresh;            // 强制刷新标志
    double mMaxFrameDuration;     // 最大帧延时
    int mFrameTimerRefresh;       // 刷新时钟
    double mFrameTimer;           // 视频时钟

    VideoDevice *mVideoDevice;    // 视频输出设备

    AVFrame *pFrameARGB;         //
    uint8_t *mBuffer;             //
    SwsContext *swsContext;      //
};

#endif //FFMPEG4_MEDIASYNC_H
