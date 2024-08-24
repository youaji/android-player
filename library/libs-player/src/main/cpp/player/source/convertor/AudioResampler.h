#ifndef FFMPEG4_AUDIORESAMPLER_H
#define FFMPEG4_AUDIORESAMPLER_H

#include <PlayerState.h>
#include <MediaSync.h>
#include <SoundTouchWrapper.h>
#include <AudioDevice.h>
#include "AndroidLog.h"

/**
 * 音频参数
 */
typedef struct AudioParams {
    int freq;                   //
    int channels;               //
    int64_t channel_layout;     //
    enum AVSampleFormat fmt;    //
    int frame_size;             //
    int bytes_per_sec;          //
} AudioParams;

/**
 * 音频重采样状态结构体
 */
typedef struct AudioState {
    double audioClock;                      // 音频时钟
    double audio_diff_cum;                  //
    double audio_diff_avg_coef;             //
    double audio_diff_threshold;            //
    int audio_diff_avg_count;               //
    int audio_hw_buf_size;                  // SLES 中音频缓冲区大小
    uint8_t *outputBuffer;                  // 输出缓冲大小
    uint8_t *resample_buffer;               // 重采样大小
    short *sound_touch_buffer;              // SoundTouch 缓冲
    unsigned int buffer_size;               // 缓冲大小
    unsigned int resample_size;             // 重采样大小
    unsigned int sound_touch_buffer_size;   // SoundTouch 处理后的缓冲大小大小
    int buffer_index;                       //
    int write_buffer_size;                  // 写入大小
    SwrContext *swr_ctx;                    // 音频转码上下文
    int64_t audio_callback_time;            // 音频回调时间
    AudioParams audio_params_src;           // 音频原始参数
    AudioParams audio_params_target;        // 音频目标参数
} AudioState;

/**
 * 音频重采样器
 */
class AudioResampler {
public:
    /**
     * @param playerState
     * @param audioDecoder
     * @param mediaSync
     */
    AudioResampler(PlayerState *playerState, AudioDecoder *audioDecoder, MediaSync *mediaSync);

    //
    virtual ~AudioResampler();

    /**
     * @param spec
     * @param wanted_channel_layout
     * @return
     */
    int setResampleParams(AudioDeviceSpec *spec, int64_t wanted_channel_layout);

    /**
     * PCM队列回调方法，用于取得PCM数据
     * @param stream
     * @param len 需要读取数据的长度
     */
    void pcmQueueCallback(uint8_t *stream, int len);

private:
    /**
     * @param nbSamples
     * @return
     */
    int audioSynchronize(int nbSamples);

    /**
     * @return
     */
    int audioFrameResample();

private:
    PlayerState *mPlayerState;               //
    MediaSync *mMediaSync;                   //
    AVFrame *mFrame;                         //
    AudioDecoder *mAudioDecoder;             // 音频解码器
    AudioState *mAudioState;                 // 音频重采样状态
    SoundTouchWrapper *mSoundTouchWrapper;   // 变速变调处理
};

#endif //FFMPEG4_AUDIORESAMPLER_H
