#ifndef FFMPEG4_SLESDEVICE_H
#define FFMPEG4_SLESDEVICE_H

#include "AudioDevice.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <pthread.h>

/**
 */
class SLESDevice : public AudioDevice {
public:
    SLESDevice();

    virtual ~SLESDevice();

    int open(const AudioDeviceSpec *desired, AudioDeviceSpec *obtained) override;

    void start() override;

    void stop() override;

    void pause() override;

    void resume() override;

    void flush() override;

    void setVolume(float volume) override;

    float getVolume() override;

    void setStereoVolume(float left_volume, float right_volume) override;

    virtual void run() override;

private:
    /**
     * 转换成SL采样率
     * @param sampleRate
     * @return
     */
    SLuint32 getSLSampleRate(int sampleRate);

    /**
     * 计算SL的音量 获取SLES音量
     * @param volumeLevel
     * @return
     */
    SLmillibel getAmplificationLevel(float volumeLevel);

private:

    SLObjectItf mSLObject;   // 引擎接口
    SLEngineItf mSLEngine;   // 引擎接口

    SLObjectItf mSLOutputMixObject;  // 混音器

    SLObjectItf mSLPlayerObject; // 播放器对象
    SLPlayItf mSLPlayItf;        // 播放器对象
    SLVolumeItf mSLVolumeItf;    // 播放器对象

    SLAndroidSimpleBufferQueueItf mSLBufferQueueItf; // 缓冲器队列接口

    AudioDeviceSpec mAudioDeviceSpec;   // 音频设备参数
    int m_bytes_per_frame;              // 一帧占多少字节
    int m_milli_per_buffer;             // 一个缓冲区时长占多少
    int m_frames_per_buffer;            // 一个缓冲区有多少帧
    int m_bytes_per_buffer;             // 一个缓冲区的大小
    uint8_t *mBuffer;                   // 缓冲区
    size_t m_buffer_capacity;           // 缓冲区总大小

    Mutex mMutex;           //
    Condition mCondition;   //
    Thread *mAudioThread;   // 音频播放线程
    int mAbortRequest;      // 终止标志
    int mPauseRequest;      // 暂停标志
    int mFlushRequest;      // 刷新标志

    bool mUpdateVolume;     // 更新音量
    float mLeftVolume;      // 左音量
    float mRightVolume;     // 右音量

};

#endif //FFMPEG4_SLESDEVICE_H
