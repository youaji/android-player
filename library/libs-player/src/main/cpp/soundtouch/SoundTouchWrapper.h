#ifndef FFMPEG4_SOUNDTOUCHWRAPPER_H
#define FFMPEG4_SOUNDTOUCHWRAPPER_H

#include <stdint.h>
#include "include/SoundTouch.h"

using namespace std;

using namespace soundtouch;

class SoundTouchWrapper {

public:
    SoundTouchWrapper();
    virtual ~SoundTouchWrapper();

    /// 初始化
    void create();

    /// 销毁
    void destroy();

    /**
     * 转换
     * @param data              待采样 PCM 数据
     * @param speed             速度
     * @param pitch             音调
     * @param len               长度
     * @param bytes_per_sample  采样字节数
     * @param n_channel         声道数
     * @param n_sampleRate      采样率
     * @return
     */
    int translate(short *data, float speed, float pitch, int len, int bytes_per_sample, int n_channel, int n_sampleRate);

    /// 获取SoundTouch对象
    SoundTouch *getSoundTouch();

private:
    SoundTouch *mSoundTouch;
};


#endif //FFMPEG4_SOUNDTOUCHWRAPPER_H
