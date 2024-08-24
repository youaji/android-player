#ifndef FFMPEG4_MEDIAPLAYER_H
#define FFMPEG4_MEDIAPLAYER_H

#include "MediaClock.h"
#include "SoundTouchWrapper.h"
#include "PlayerState.h"
#include "AudioDecoder.h"
#include "VideoDecoder.h"

#if defined(__ANDROID__)

#include "SLESDevice.h"
#include "GLESDevice.h"

#else

#include <device/AudioDevice.h>
#include <device/VideoDevice.h>

#endif

#include <android/native_window.h>
#include <android/native_window_jni.h>
#include "MediaSync.h"
#include "convertor/AudioResampler.h"
#include "recorder/VideoRecorder.h"
#include "recorder/ScreenshotRecorder.h"


class MediaPlayer : public Runnable {
public:
    MediaPlayer();

    virtual ~MediaPlayer();

    status_t reset();

    void setDataSource(const char *url, int64_t offset = 0, const char *headers = NULL);

    void setVideoDevice(VideoDevice *videoDevice);

    status_t prepare();

    status_t prepareAsync();

    void start();

    void pause();

    void resume();

    void stop();

    void seekTo(float timeMs);

    void setLooping(int looping);

    void setVolume(float volume);

    float getVolume();

    void setStereoVolume(float leftVolume, float rightVolume);

    void setMute(int mute);

    void setRate(float rate);

    void setPitch(float pitch);

    int getRotate();

    int getVideoWidth();

    int getVideoHeight();

    long getCurrentPosition();

    long getDuration();

    int isPlaying();

    int isLooping();

    int getMetadata(AVDictionary **metadata);

    AVMessageQueue *getMessageQueue();

    PlayerState *getPlayerState();

    void pcmQueueCallback(uint8_t *stream, int len);

    /**
    * 开始录制
    * @param
    * @return 是否成功
    */
    int startRecord(const char *filePath);

    /**
     * 停止录制
     * @return 是否成功
     */
    int stopRecord();
//    void closeOutput();

    /**
     * 是否录制中
     * @return 是否成功
     */
    int isRecording();

    /**
     * 捕获当前帧
     * @param filePath
     * @return 是否成功
     */
    int screenshot(const char *filePath);

protected:
    void run() override;

private:
    /**
     * 通知出错
     * @param message
     */
    void notifyErrorMessage(const char *message);

    /**
     * 解封装
     * @return
     */
    int demux();

    /**
     * 准备解码器
     * @return
     */
    int prepareDecoder();

    /**
     * 开始解码
     */
    void startDecode();

    /**
     * 打开多媒体设备
     * @return
     */
    int openMediaDevice();

    /**
     * 获取AV数据
     * @return
     */
    int readAVPackets();

    /**
     * @return
     */
    int startPlayer();

    /**
     * prepare decoder with stream_index
     * @param streamIndex
     * @return
     */
    int prepareDecoder(int streamIndex);

    /**
     * open an audio output device
     * @param wanted_channel_layout
     * @param wanted_nb_channels
     * @param wanted_sample_rate
     * @return
     */
    int openAudioDevice(
            int64_t wanted_channel_layout,
            int wanted_nb_channels,
            int wanted_sample_rate
    );

private:
    Mutex mMutex;
    Condition mCondition;
    Thread *mReadThread;                     // 读数据包线程
    PlayerState *mPlayerState;               // 播放器状态
    AudioDecoder *mAudioDecoder;             // 音频解码器
    VideoDecoder *mVideoDecoder;             // 视频解码器
    bool mIsExit;                            // state for reading packets thread exited if not
    // 解复用处理
    AVFormatContext *mFormatCtx;             // 解码上下文
    int64_t mDuration;                       // 文件总时长，单位毫秒
    int mLastPaused;                         // 上一次暂停状态
    int mEOF;                                // 数据包读到结尾标志
    int mAttachmentRequest;                  // 视频封面数据包请求

    AudioDevice *mAudioDevice;               // 音频输出设备
    AudioResampler *mAudioResampler;         // 音频重采样器

    MediaSync *mMediaSync;                   // 媒体同步器

//    bool mIsRecording = false;               // 录制中
//    bool mRequestScreenshot = false;         // 截图捕获

    VideoRecorder *mVideoRecorder;
//    ScreenshotRecorder *mScreenshotRecorder;
//
//    AVFormatContext *mOutputCtx;
};

#endif //FFMPEG4_MEDIAPLAYER_H
