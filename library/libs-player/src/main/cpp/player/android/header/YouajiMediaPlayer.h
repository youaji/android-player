#ifndef FFMPEG4_YOUAJIMEDIAPLAYER_H
#define FFMPEG4_YOUAJIMEDIAPLAYER_H

#include <AndroidLog.h>
#include <Mutex.h>
#include <Condition.h>
#include <Thread.h>
#include <android/native_window.h>
#include <libavutil/dict.h>
#include <GLESDevice.h>
#include <MediaPlayer.h>

enum media_event_type {
    MEDIA_NOP = 0, // interface test message
    MEDIA_PREPARED = 1,
    MEDIA_PLAYBACK_COMPLETE = 2,
    MEDIA_BUFFERING_UPDATE = 3,
    MEDIA_SEEK_COMPLETE = 4,
    MEDIA_SET_VIDEO_SIZE = 5,
    MEDIA_STARTED = 6,
    MEDIA_TIMED_TEXT = 99,
    MEDIA_ERROR = 100,
    MEDIA_INFO = 200,
    MEDIA_CURRENT = 300,

    MEDIA_SET_VIDEO_SAR = 10001
};

enum media_error_type {
    // 0xx
    MEDIA_ERROR_UNKNOWN = 1,

    // 1xx
    MEDIA_ERROR_SERVER_DIED = 100,

    // 2xx
    MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK = 200,
};

enum media_info_type {
    // 0xx
    MEDIA_INFO_UNKNOWN = 1,
    // The player was started because it was used as the next player for another player, which just completed playback
    MEDIA_INFO_STARTED_AS_NEXT = 2,
    // The player just pushed the very first video frame for rendering
    MEDIA_INFO_RENDERING_START = 3,

    // 7xx
    // The video is too complex for the decoder: it can't decode frames fast enough. Possibly only the audio plays fine at this stage.
    MEDIA_INFO_VIDEO_TRACK_LAGGING = 700,
    // MediaPlayer is temporarily pausing playback internally in order to buffer more data.
    MEDIA_INFO_BUFFERING_START = 701,
    // MediaPlayer is resuming playback after filling buffers.
    MEDIA_INFO_BUFFERING_END = 702,
    // Bandwidth in recent past
    MEDIA_INFO_NETWORK_BANDWIDTH = 703,

    // 8xx
    // Bad interleaving means that a media has been improperly interleaved or not interleaved at all,
    // e.g has all the video samples first then all the audio ones.
    // Video is playing but a lot of disk seek may be happening.
    MEDIA_INFO_BAD_INTERLEAVING = 800,
    // The media is not seekable (e.g live stream).
    MEDIA_INFO_NOT_SEEKABLE = 801,
    // New media metadata is available.
    MEDIA_INFO_METADATA_UPDATE = 802,
    // Audio can not be played.
    MEDIA_INFO_PLAY_AUDIO_ERROR = 804,
    // Video can not be played.
    MEDIA_INFO_PLAY_VIDEO_ERROR = 805,

    //9xx
    MEDIA_INFO_TIMED_TEXT_ERROR = 900,
};

/**
 * 抽象类，类似于Java的接口，notify方法留给实现类去定义
 */
class MediaPlayerListener {
public:
    virtual void notify(int msg, int ext1, int ext2, void *obj) {} // 定义一个虚函数，留给继承者实现
};

/**
 */
class YouajiMediaPlayer : public Runnable {
public:
    /**
     */
    YouajiMediaPlayer();

    /**
     */
    virtual ~YouajiMediaPlayer();

    /**
     */
    void init();

    /**
     */
    void disconnect();

    /**
     * @param url
     * @param offset
     * @param headers
     * @return
     */
    status_t setDataSource(const char *url, int64_t offset = 0, const char *headers = NULL);

    /**
     *
     * @param allow
     * @param block
     * @return
     */
    status_t setMetadataFilter(char *allow[], char *block[]);

    /**
     *
     * @param update_only
     * @param apply_filter
     * @param metadata
     * @return
     */
    status_t getMetadata(bool update_only, bool apply_filter, AVDictionary **metadata);

    /**
     *
     * @param native_window
     * @return
     */
    status_t setVideoSurface(ANativeWindow *native_window);

    /**
     * Surface的大小发生改变
     * @param width
     * @param height
     */
    void surfaceChanged(int width, int height);

    /**
     * 改变滤镜
     * @param type
     * @param name
     */
    void changeFilter(int type, const char *name);

    /**
    * 改变滤镜
    * @param type
    * @param id
    */
    void changeFilter(int type, const int id);

    /**
     * 设置水印
     * @param watermarkPixel
     * @param length
     * @param width
     * @param height
     * @param scale
     * @param location
     */
    void setWatermark(uint8_t *watermarkPixel, size_t length, GLint width, GLint height, GLfloat scale, GLint location);

    /**
     * @param listener
     * @return
     */
    status_t setListener(MediaPlayerListener *listener);

    /**
     * @return
     */
    status_t prepare();

    /**
     * @return
     */
    status_t prepareAsync();

    /**
     */
    void start();

    /**
     */
    void stop();

    /**
     */
    void pause();

    /**
     */
    void resume();

    /**
     * @return
     */
    bool isPlaying();

    /**
     * @return
     */
    int getRotate();

    /**
     * @return
     */
    int getVideoWidth();

    /**
     * @return
     */
    int getVideoHeight();

    /**
     * @param msec
     * @return
     */
    status_t seekTo(float msec);

    /**
     * @return
     */
    long getCurrentPosition();

    /**
     * @return
     */
    long getDuration();

    /**
     * @return
     */
    status_t reset();

    /**
     * @param type
     * @return
     */
    status_t setAudioStreamType(int type);

    /**
     * @param looping
     * @return
     */
    status_t setLooping(bool looping);

    /**
     * @return
     */
    bool isLooping();

    /**
     * setVolume
     * @param volume
     * @return
     */
    status_t setVolume(float volume);

    /**
     * @return volume
     */
    float getVolume();

    /**
     * @param leftVolume
     * @param rightVolume
     * @return
     */
    status_t setStereoVolume(float leftVolume, float rightVolume);

    /**
     * @param mute
     */
    void setMute(bool mute);

    /**
     * @param speed
     */
    void setRate(float speed);

    /**
     * @param pitch
     */
    void setPitch(float pitch);

    /**
     * @param sessionId
     * @return
     */
    status_t setAudioSessionId(int sessionId);

    /**
     * @return
     */
    int getAudioSessionId();

    /**
     *
     * @param category
     * @param type
     * @param option
     */
    void setOption(int category, const char *type, const char *option);

    /**
     *
     * @param category
     * @param type
     * @param option
     */
    void setOption(int category, const char *type, int64_t option);

    /**
     * @param msg
     * @param ext1
     * @param ext2
     * @param obj
     * @param len
     */
    void notify(int msg, int ext1, int ext2, void *obj = NULL, int len = 0);

    /**
     * 开始录制
     * @param
     * @return 是否成功
     */
    bool startRecord(const char *filePath);
    /**
     * 停止录制
     * @return 是否成功
     */
    bool stopRecord();

    /**
     * 是否录制中
     * @return 是否成功
     */
    bool isRecording();

    /**
     * 捕获当前帧
     * @param filePath
     * @return 是否成功
     */
    bool screenshot(const char *filePath);

protected:
    //override表示重写了基类的虚函数
    void run() override;

private:
    /**
     * @param what
     * @param arg1
     * @param arg2
     * @param obj
     */
    void postEvent(int what, int arg1, int arg2, void *obj = NULL);

private:
    Mutex mMutex;
    Condition mCondition;
    Thread *mMessageThread;
    bool mIsAbortRequest;
    GLESDevice *mVideoDevice;
    MediaPlayer *mMediaPlayer;
    MediaPlayerListener *mMediaPlayerListener;

    bool mIsSeeking;
    long mSeekingPosition;
    bool mIsPrepareSync;
    status_t mPrepareStatus;
    int mAudioSessionId;
};

#endif //FFMPEG4_YOUAJIMEDIAPLAYER_H