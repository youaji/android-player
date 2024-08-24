#ifndef FFMPEG4_AUDIODEVICE_H
#define FFMPEG4_AUDIODEVICE_H

#include "PlayerState.h"

/**
 * 音频PCM填充回调\n
 * 首先这是一个函数指针，指向一个返回 void，参数如下所示的函数\n
 * 然后用 typedef 给这个函数指针起一个别名为 AudioPCMCallback
 */
typedef void (*AudioPCMCallback)(void *userdata, uint8_t *stream, int len);

typedef struct AudioDeviceSpec {
    int freq;                   // 采样率
    AVSampleFormat format;      // 音频采样格式
    uint8_t channels;           // 声道
    uint16_t samples;           // 采样大小
    uint32_t size;              // 缓冲区大小
    AudioPCMCallback callback;  // 音频回调，取得音频的PCM数据
    void *userdata;             // 音频上下文
} AudioDeviceSpec;

/**
 */
class AudioDevice : public Runnable {
public:
    /**
     */
    AudioDevice();

    /**
     */
    virtual ~AudioDevice();

    /**
     * 打开音频设备，并返回缓冲区大小
     * @param desired   期望的参数设置
     * @param obtained
     * @return
     */
    virtual int open(const AudioDeviceSpec *desired, AudioDeviceSpec *obtained);

    /**
     */
    virtual void start();

    /**
     */
    virtual void stop();

    /**
     */
    virtual void pause();

    /**
     */
    virtual void resume();

    /**
     * 清空SL缓冲队列
     */
    virtual void flush();

    /**
     * 设置音量
     * @param volume
     */
    virtual void setVolume(float volume);

    /**
    * 获取音量
    * @param volume
    */
    virtual float getVolume();

    /**
     * 设置音量
     * @param left_volume
     * @param right_volume
     */
    virtual void setStereoVolume(float left_volume, float right_volume);

    /**
     */
    virtual void run();
};

#endif //FFMPEG4_AUDIODEVICE_H
