#include <AndroidLog.h>
#include "SLESDevice.h"

#define OPENSLES_BUFFERS 4  // 最大缓冲区数量
#define OPENSLES_BUFLEN  10 // 缓冲区长度(毫秒)

SLESDevice::SLESDevice() {
    mSLObject = NULL;
    mSLEngine = NULL;
    mSLOutputMixObject = NULL;
    mSLPlayerObject = NULL;
    mSLPlayItf = NULL;
    mSLVolumeItf = NULL;
    mSLBufferQueueItf = NULL;
    memset(&mAudioDeviceSpec, 0, sizeof(AudioDeviceSpec));
    mAbortRequest = 1;
    mPauseRequest = 0;
    mFlushRequest = 0;
    mAudioThread = NULL;
    mUpdateVolume = false;
}

SLESDevice::~SLESDevice() {
    mMutex.lock();
    memset(&mAudioDeviceSpec, 0, sizeof(AudioDeviceSpec));
    if (mSLPlayerObject != NULL) {
        (*mSLPlayerObject)->Destroy(mSLPlayerObject);
        mSLPlayerObject = NULL;
        mSLPlayItf = NULL;
        mSLVolumeItf = NULL;
        mSLBufferQueueItf = NULL;
    }

    if (mSLOutputMixObject != NULL) {
        (*mSLOutputMixObject)->Destroy(mSLOutputMixObject);
        mSLOutputMixObject = NULL;
    }

    if (mSLObject != NULL) {
        (*mSLObject)->Destroy(mSLObject);
        mSLObject = NULL;
        mSLEngine = NULL;
    }
    mMutex.unlock();
}

void SLESDevice::start() {
    // 回调存在时，表示成功打开SLES音频设备，另外开一个线程播放音频
    if (mAudioDeviceSpec.callback != NULL) {
        mAbortRequest = 0;
        mPauseRequest = 0;
        if (!mAudioThread) {
            mAudioThread = new Thread(this, Priority_High);
            mAudioThread->start();
        }
    } else {
        LOGE("SLESDevice->audio device callback is NULL!");
    }
}

void SLESDevice::stop() {
    mMutex.lock();
    mAbortRequest = 1;
    mCondition.signal();
    mMutex.unlock();

    if (mAudioThread) {
        mAudioThread->join();
        delete mAudioThread;
        LOGD("SLESDevice->删除音频设备线程");
        mAudioThread = NULL;
    }
}

void SLESDevice::pause() {
    mMutex.lock();
    mPauseRequest = 1;
    mCondition.signal();
    mMutex.unlock();
}

void SLESDevice::resume() {
    mMutex.lock();
    mPauseRequest = 0;
    mCondition.signal();
    mMutex.unlock();
}

void SLESDevice::flush() {
    mMutex.lock();
    mFlushRequest = 1;
    mCondition.signal();
    mMutex.unlock();
}

void SLESDevice::setVolume(float volume) {
    setStereoVolume(volume, volume);
}

float SLESDevice::getVolume() {
    // TODO:数据有问题，获取到的音量总是为0，或者负数
    SLmillibel level = 0;
    if (mSLVolumeItf != NULL) {
        SLresult result = (*mSLVolumeItf)->GetVolumeLevel(mSLVolumeItf, &level);
        if (result != SL_RESULT_SUCCESS) {
            LOGE("SLESDevice->mSLVolumeItf->GetVolumeLevel failed %d\n", (int) result);
        }
    } else {
        LOGE("SLESDevice->mSLVolumeItf->GetVolumeLevel, mSLVolumeItf is null");
    }
    LOGD("SLESDevice->mSLVolumeItf->GetVolumeLevel->%d", level);
    mLeftVolume = mRightVolume = level;
    return mLeftVolume;
}

void SLESDevice::setStereoVolume(float left_volume, float right_volume) {
    Mutex::Autolock lock(mMutex);
    if (!mUpdateVolume) {
        mLeftVolume = left_volume;
        mRightVolume = right_volume;
        mUpdateVolume = true;
    }
    mCondition.signal();
}

void SLESDevice::run() {
    uint8_t *next_buffer = NULL;
    int next_buffer_index = 0;

    // 设置播放状态
    if (!mAbortRequest && !mPauseRequest) {
        (*mSLPlayItf)->SetPlayState(mSLPlayItf, SL_PLAYSTATE_PLAYING);
    }

    while (true) {

        // 退出播放线程
        if (mAbortRequest) {
            break;
        }

        // 暂停
        if (mPauseRequest) {
            continue;
        }

        // 获取缓冲队列状态
        SLAndroidSimpleBufferQueueState slState = {0};
        SLresult slRet = (*mSLBufferQueueItf)->GetState(mSLBufferQueueItf, &slState);
        if (slRet != SL_RESULT_SUCCESS) {
            LOGE("SLESDevice->%s: mSLBufferQueueItf->GetState() failed\n", __func__);
            mMutex.unlock();
        }

        // 判断暂停或者队列中缓冲区填满了
        mMutex.lock();
        if (!mAbortRequest && (mPauseRequest || slState.count >= OPENSLES_BUFFERS)) { //暂停或者缓冲队列满了
            while (!mAbortRequest && (mPauseRequest || slState.count >= OPENSLES_BUFFERS)) {
                // LOGE("SLESDevice->音频暂停%d=",mPauseRequest);
                // 非暂停
                if (!mPauseRequest) {
                    (*mSLPlayItf)->SetPlayState(mSLPlayItf, SL_PLAYSTATE_PLAYING);
                }
                // 等待
                mCondition.waitRelative(mMutex, 10 * 1000000);
                slRet = (*mSLBufferQueueItf)->GetState(mSLBufferQueueItf, &slState);
                if (slRet != SL_RESULT_SUCCESS) {
                    LOGE("SLESDevice->%s: mSLBufferQueueItf->GetState() failed\n", __func__);
                    mMutex.unlock();
                }
                // 暂停
                if (mPauseRequest) {
                    (*mSLPlayItf)->SetPlayState(mSLPlayItf, SL_PLAYSTATE_PAUSED);
                }
            }

            if (!mAbortRequest && !mPauseRequest) {
                (*mSLPlayItf)->SetPlayState(mSLPlayItf, SL_PLAYSTATE_PLAYING);
            }

        }

        if (mFlushRequest) {
            (*mSLBufferQueueItf)->Clear(mSLBufferQueueItf);
            mFlushRequest = 0;
        }
        mMutex.unlock();

        mMutex.lock();
        // 通过回调填充PCM数据
        if (mAudioDeviceSpec.callback != NULL) {
            // LOGE("SLESDevice->取音频数据");
            // buffer是缓存区的总大小，m_bytes_per_buffer是一个缓冲区的大小，这里一共有4个
            next_buffer = mBuffer + next_buffer_index * m_bytes_per_buffer;
            next_buffer_index = (next_buffer_index + 1) % OPENSLES_BUFFERS;
            // 通过回调函数取数据，即将数据保存到sl的缓冲区
            mAudioDeviceSpec.callback(mAudioDeviceSpec.userdata, next_buffer, m_bytes_per_buffer);
        }
        mMutex.unlock();

        // 是否要更新音量
        if (mUpdateVolume) {
            if (mSLVolumeItf != NULL) {
                SLmillibel level = getAmplificationLevel((mLeftVolume + mRightVolume) / 2);
                SLresult result = (*mSLVolumeItf)->SetVolumeLevel(mSLVolumeItf, level);
                if (result != SL_RESULT_SUCCESS) {
                    LOGE("SLESDevice->mSLVolumeItf->SetVolumeLevel failed %d\n", (int) result);
                }
            }
            mUpdateVolume = false;
        }

        // 刷新缓冲区还是将数据入队缓冲区
        if (mFlushRequest) {
            (*mSLBufferQueueItf)->Clear(mSLBufferQueueItf);
            mFlushRequest = 0;
        } else {
            if (mSLPlayItf != NULL) {
                (*mSLPlayItf)->SetPlayState(mSLPlayItf, SL_PLAYSTATE_PLAYING);
            }
            // 数据入队缓冲区
            slRet = (*mSLBufferQueueItf)->Enqueue(mSLBufferQueueItf, next_buffer, m_bytes_per_buffer);
            if (slRet == SL_RESULT_SUCCESS) {
                // do nothing
            } else if (slRet == SL_RESULT_BUFFER_INSUFFICIENT) {
                // don't retry, just pass through
                LOGE("SLESDevice->SL_RESULT_BUFFER_INSUFFICIENT\n");
            } else {
                LOGE("SLESDevice->mSLBufferQueueItf->Enqueue() = %d\n", (int) slRet);
                break;
            }
        }
    }

    if (mSLPlayItf) {
        (*mSLPlayItf)->SetPlayState(mSLPlayItf, SL_PLAYSTATE_STOPPED);
    }
}


/**
 * SLES缓冲回调
 * @param bf
 * @param context
 */
void slBufferPCMCallBack(SLAndroidSimpleBufferQueueItf bf, void *context) {}

int SLESDevice::open(const AudioDeviceSpec *desired, AudioDeviceSpec *obtained) {
    LOGD("SLESDevice->打开音频");
    SLresult result;
    // 创建引擎engineObject
    result = slCreateEngine(&mSLObject, 0, NULL, 0, NULL, NULL);
    if ((result) != SL_RESULT_SUCCESS) {
        LOGE("SLESDevice->%s: slCreateEngine() failed", __func__);
        return -1;
    }
    // 实现引擎engineObject
    result = (*mSLObject)->Realize(mSLObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("SLESDevice->%s: mSLObject->Realize() failed", __func__);
        return -1;
    }
    // 获取引擎接口SLEngineItf
    result = (*mSLObject)->GetInterface(mSLObject, SL_IID_ENGINE, &mSLEngine);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("SLESDevice->%s: mSLObject->GetInterface() failed", __func__);
        return -1;
    }

    const SLInterfaceID mids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean mreq[1] = {SL_BOOLEAN_FALSE};
    // 创建混音器outputMixObject
    result = (*mSLEngine)->CreateOutputMix(mSLEngine, &mSLOutputMixObject, 1, mids, mreq);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("SLESDevice->%s: mSLEngine->CreateOutputMix() failed", __func__);
        return -1;
    }
    // 实现混音器outputMixObject
    result = (*mSLOutputMixObject)->Realize(mSLOutputMixObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("SLESDevice->%s: mSLOutputMixObject->Realize() failed", __func__);
        return -1;
    }
    // 设置混音器
    SLDataLocator_OutputMix outputMix = {
            SL_DATALOCATOR_OUTPUTMIX,
            mSLOutputMixObject
    };
    SLDataSink audioSink = {
            &outputMix,
            NULL
    };
    /*
    typedef struct SLDataLocator_AndroidBufferQueue_ {
    SLuint32    locatorType;//缓冲区队列类型
    SLuint32    numBuffers;//buffer位数
    }*/
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {
            SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
            OPENSLES_BUFFERS
    };

    // 根据通道数设置通道mask
    SLuint32 channelMask;
    switch (desired->channels) {
        case 2: {
            channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
            break;
        }
        case 1: {
            channelMask = SL_SPEAKER_FRONT_CENTER;
            break;
        }
        default: {
            LOGE("SLESDevice->%s, invalid channel %d", __func__, desired->channels);
            return -1;
        }
    }

    // pcm格式
    SLDataFormat_PCM format_pcm = {
            SL_DATAFORMAT_PCM,              // 播放器PCM格式
            desired->channels,              // 声道数
            getSLSampleRate(desired->freq), // SL采样率
            SL_PCMSAMPLEFORMAT_FIXED_16,    // 位数 16位
            SL_PCMSAMPLEFORMAT_FIXED_16,    // 和位数一致
            channelMask,                    // 声道
            SL_BYTEORDER_LITTLEENDIAN       // 小端存储
    };
    // 新建一个数据源 将上述配置信息放到这个数据源中
    SLDataSource slDataSource = {
            &android_queue,
            &format_pcm
    };

    const SLInterfaceID ids[3] = {
            SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
            SL_IID_VOLUME,
            SL_IID_PLAY
    };
    const SLboolean req[3] = {
            SL_BOOLEAN_TRUE,
            SL_BOOLEAN_TRUE,
            SL_BOOLEAN_TRUE
    };

    /* 创建一个音频播放器
      SLresult (*CreateAudioPlayer) (
     SLEngineItf self,
     SLObjectItf * pPlayer,
     SLDataSource *pAudioSrc,//数据设置
     SLDataSink *pAudioSnk,//关联混音器
     SLuint32 numInterfaces,
     const SLInterfaceID * pInterfaceIds,
     const SLboolean * pInterfaceRequired
     );
     */
    result = (*mSLEngine)->CreateAudioPlayer(mSLEngine, &mSLPlayerObject, &slDataSource, &audioSink, 3, ids, req);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("SLESDevice->%s: mSLEngine->CreateAudioPlayer() failed", __func__);
        return -1;
    }
    // 初始化播放器
    result = (*mSLPlayerObject)->Realize(mSLPlayerObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("SLESDevice->%s: mSLPlayerObject->Realize() failed", __func__);
        return -1;
    }
    // 得到接口后调用，获取Player接口
    result = (*mSLPlayerObject)->GetInterface(mSLPlayerObject, SL_IID_PLAY, &mSLPlayItf);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("SLESDevice->%s: mSLPlayerObject->GetInterface(SL_IID_PLAY) failed", __func__);
        return -1;
    }
    // 获取音量接口
    result = (*mSLPlayerObject)->GetInterface(mSLPlayerObject, SL_IID_VOLUME, &mSLVolumeItf);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("SLESDevice->%s: mSLPlayerObject->GetInterface(SL_IID_VOLUME) failed", __func__);
        return -1;
    }
    // 注册回调缓冲区，通过缓冲区里面的数据进行播放
    result = (*mSLPlayerObject)->GetInterface(mSLPlayerObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &mSLBufferQueueItf);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("SLESDevice->%s: mSLPlayerObject->GetInterface(SL_IID_ANDROIDSIMPLEBUFFERQUEUE) failed", __func__);
        return -1;
    }
    // 设置回调接口，回调函数的作用应该是有数据进来的时候，马上消费，消费完马上又调用回调函数取数据
    result = (*mSLBufferQueueItf)->RegisterCallback(mSLBufferQueueItf, slBufferPCMCallBack, this);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("SLESDevice->%s: mSLBufferQueueItf->RegisterCallback() failed", __func__);
        return -1;
    }

    // 这里计算缓冲区大小等参数，其实frames_per_buffer应该是一个缓冲区有多少个采样点的意思
    m_bytes_per_frame = format_pcm.numChannels * format_pcm.bitsPerSample / 8;      // 一帧占多少字节
    m_milli_per_buffer = OPENSLES_BUFLEN;                                           // 每个缓冲区占多少毫秒
    m_frames_per_buffer = m_milli_per_buffer * format_pcm.samplesPerSec / 1000000;  // 一个缓冲区有多少帧数据
    m_bytes_per_buffer = m_bytes_per_frame * m_frames_per_buffer;                   // 一个缓冲区大小
    m_buffer_capacity = OPENSLES_BUFFERS * m_bytes_per_buffer;                      // 总共的缓存容量

    LOGD("SLESDevice->m_bytes_per_frame  = %d bytes\n", m_bytes_per_frame);
    LOGD("SLESDevice->m_milli_per_buffer = %d ms\n", m_milli_per_buffer);
    LOGD("SLESDevice->m_frame_per_buffer = %d frames\n", m_frames_per_buffer);
    LOGD("SLESDevice->m_buffer_capacity  = %d bytes\n", m_buffer_capacity);
    LOGD("SLESDevice->m_buffer_capacity  = %d bytes\n", (int) m_buffer_capacity);

    if (obtained != NULL) {
        *obtained = *desired;
        obtained->size = (uint32_t) m_buffer_capacity;
        obtained->freq = format_pcm.samplesPerSec / 1000;
    }
    mAudioDeviceSpec = *desired;

    // 创建缓冲区
    mBuffer = (uint8_t *) malloc(m_buffer_capacity);
    if (!mBuffer) {
        LOGE("SLESDevice->%s: failed to alloc buffer %d\n", __func__, (int) m_buffer_capacity);
        return -1;
    }

    // 填充缓冲区数据
    memset(mBuffer, 0, m_buffer_capacity);
    // 分配缓冲队列
    for (int i = 0; i < OPENSLES_BUFFERS; ++i) {
        result = (*mSLBufferQueueItf)->Enqueue(mSLBufferQueueItf, mBuffer + i * m_bytes_per_buffer, m_bytes_per_buffer);
        if (result != SL_RESULT_SUCCESS) {
            LOGE("SLESDevice->%s: mSLBufferQueueItf->Enqueue(000...) failed", __func__);
        }
    }

    LOGD("SLESDevice->open SLES Device success");
    // 返回缓冲大小
    return m_buffer_capacity;
}

SLuint32 SLESDevice::getSLSampleRate(int sampleRate) {
    switch (sampleRate) {
        case 8000: {
            return SL_SAMPLINGRATE_8;
        }
        case 11025: {
            return SL_SAMPLINGRATE_11_025;
        }
        case 12000: {
            return SL_SAMPLINGRATE_12;
        }
        case 16000: {
            return SL_SAMPLINGRATE_16;
        }
        case 22050: {
            return SL_SAMPLINGRATE_22_05;
        }
        case 24000: {
            return SL_SAMPLINGRATE_24;
        }
        case 32000: {
            return SL_SAMPLINGRATE_32;
        }
        case 44100: {
            return SL_SAMPLINGRATE_44_1;
        }
        case 48000: {
            return SL_SAMPLINGRATE_48;
        }
        case 64000: {
            return SL_SAMPLINGRATE_64;
        }
        case 88200: {
            return SL_SAMPLINGRATE_88_2;
        }
        case 96000: {
            return SL_SAMPLINGRATE_96;
        }
        case 192000: {
            return SL_SAMPLINGRATE_192;
        }
        default: {
            return SL_SAMPLINGRATE_44_1;
        }
    }
}

SLmillibel SLESDevice::getAmplificationLevel(float volumeLevel) {
    if (volumeLevel < 0.00000001) {
        return SL_MILLIBEL_MIN;
    }
    SLmillibel mb = lroundf(2000.f * log10f(volumeLevel));
    if (mb < SL_MILLIBEL_MIN) {
        mb = SL_MILLIBEL_MIN;
    } else if (mb > 0) {
        mb = 0;
    }
    return mb;
}