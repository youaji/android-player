#include "AudioResampler.h"

AudioResampler::AudioResampler(PlayerState *playerState, AudioDecoder *audioDecoder, MediaSync *mediaSync) {
    this->mPlayerState = playerState;
    this->mAudioDecoder = audioDecoder;
    this->mMediaSync = mediaSync;
    // 音频重采样结构体
    mAudioState = (AudioState *) av_mallocz(sizeof(AudioState));
    memset(mAudioState, 0, sizeof(AudioState));
    mSoundTouchWrapper = new SoundTouchWrapper();
    mFrame = av_frame_alloc();

}

AudioResampler::~AudioResampler() {
    mPlayerState = NULL;
    mAudioDecoder = NULL;
    mMediaSync = NULL;
    if (mSoundTouchWrapper) {
        delete mSoundTouchWrapper;
        mSoundTouchWrapper = NULL;
    }
    if (mAudioState) {
        swr_free(&mAudioState->swr_ctx);
        av_freep(&mAudioState->resample_buffer);
        memset(mAudioState, 0, sizeof(AudioState));
        av_free(mAudioState);
        mAudioState = NULL;
    }
    if (mFrame) {
        av_frame_unref(mFrame);
        av_frame_free(&mFrame);
        mFrame = NULL;
    }
}

int AudioResampler::setResampleParams(AudioDeviceSpec *spec, int64_t wanted_channel_layout) {

    mAudioState->audio_params_src = mAudioState->audio_params_target;
    mAudioState->audio_hw_buf_size = spec->size;
    mAudioState->buffer_size = 0;
    mAudioState->buffer_index = 0;
    mAudioState->audio_diff_avg_coef = exp(log(0.01) / AUDIO_DIFF_AVG_NB);
    mAudioState->audio_diff_avg_count = 0;
    mAudioState->audio_diff_threshold =
            (double) (mAudioState->audio_hw_buf_size) / mAudioState->audio_params_target.bytes_per_sec;
    mAudioState->audio_params_target.fmt = AV_SAMPLE_FMT_S16;
    mAudioState->audio_params_target.freq = spec->freq;
    mAudioState->audio_params_target.channel_layout = wanted_channel_layout;
    mAudioState->audio_params_target.channels = spec->channels;
    // 一帧的缓存大小，第三个参数为一个通道的采样点大小，所以这里计算得到的其实是一个采样点的大小，单位是byte
    mAudioState->audio_params_target.frame_size = av_samples_get_buffer_size(
            NULL,
            mAudioState->audio_params_target.channels,
            1,
            mAudioState->audio_params_target.fmt,
            1
    );
    // 期望采样达到一秒多少 bytes
    mAudioState->audio_params_target.bytes_per_sec = av_samples_get_buffer_size(
            NULL,
            mAudioState->audio_params_target.channels,
            mAudioState->audio_params_target.freq,
            mAudioState->audio_params_target.fmt,
            1
    );

//    LOGE("AudioResampler->原始参数：%d--%d--%d",mAudioState->audio_params_target.freq,mAudioState->audio_params_target.channels,mAudioState->audio_params_target.frame_size)
    if (mAudioState->audio_params_target.bytes_per_sec <= 0 ||
        mAudioState->audio_params_target.frame_size <= 0) {
        LOGE("AudioResampler->av_samples_get_buffer_size failed");
        return -1;
    }
    return 0;
}

void AudioResampler::pcmQueueCallback(uint8_t *stream, int len) {
    int bufferSize, length;
    // 没有音频解码器时，直接返回
    if (!mAudioDecoder) {
        memset(stream, 0, len);
        return;
    }
    // 单位:AV_TIME_BASE,
    // 即 ffmpeg 内部使用的时间单位，返回的可能是从系统启动那一刻开始计时的时间
    mAudioState->audio_callback_time = av_gettime_relative();
    while (len > 0) {
        // 一般 audioState->bufferSize 为一次 audioFrameResample 采集音频数据大小，
        // mAudioState->buffer_index 实际上就是这些数据写了多少
        if (mAudioState->buffer_index >= mAudioState->buffer_size) {
            // 采集的音频数据保存到 audioState->outputBuffer 缓存区中
            bufferSize = audioFrameResample();
            if (bufferSize < 0) {
                mAudioState->outputBuffer = NULL;
                mAudioState->buffer_size = (unsigned int) (
                        AUDIO_MIN_BUFFER_SIZE /
                        mAudioState->audio_params_target.frame_size *
                        mAudioState->audio_params_target.frame_size
                );
            } else {
                mAudioState->buffer_size = bufferSize;
            }
            mAudioState->buffer_index = 0;
        }

        length = mAudioState->buffer_size - mAudioState->buffer_index;
        // 如果读取长度大于需要数据的长度
        if (length > len) {
            length = len;
        }
        // 复制经过转码输出的PCM数据到缓冲区中
        if (mAudioState->outputBuffer != NULL && !mPlayerState->mute) {
            // 从存储区 str2 复制 n 个字符到存储区 str1。
            memcpy(stream, mAudioState->outputBuffer + mAudioState->buffer_index, length);
        } else {
            // 暂停执行这里
            memset(stream, 0, length);
        }
        len -= length;
        stream += length;
        // 写了多少index就增加多少
        mAudioState->buffer_index += length;
    }
    // 循环结束，表示已经写够了 len 长度的数据

    // 保存已经写入的大小
    mAudioState->write_buffer_size = mAudioState->buffer_size - mAudioState->buffer_index;

    if (!isnan(mAudioState->audioClock) && mMediaSync) {
        // audioState->audioClock 代表当前帧播放完时的时刻，
        mMediaSync->updateAudioClock(
                mAudioState->audioClock - (double) (2 * mAudioState->audio_hw_buf_size + mAudioState->write_buffer_size) / mAudioState->audio_params_target.bytes_per_sec,
                mAudioState->audio_callback_time / 1000000.0
        );
    }
}

int AudioResampler::audioSynchronize(int nbSamples) {
    int wanted_nb_samples = nbSamples;
    // 如果时钟不是同步到音频流，则需要进行对音频频进行同步处理
    if (mPlayerState->sync_type != AV_SYNC_AUDIO) {
        double diff, avg_diff;
        int min_nb_samples, max_nb_samples;
        diff = mMediaSync ? mMediaSync->getAudioDiffClock() : 0;
        if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD) {
            mAudioState->audio_diff_cum = diff + mAudioState->audio_diff_avg_coef * mAudioState->audio_diff_cum;
            if (mAudioState->audio_diff_avg_count < AUDIO_DIFF_AVG_NB) {
                mAudioState->audio_diff_avg_count++;
            } else {
                avg_diff = mAudioState->audio_diff_cum * (1.0 - mAudioState->audio_diff_avg_coef);
                if (fabs(avg_diff) >= mAudioState->audio_diff_threshold) {
                    wanted_nb_samples = nbSamples + (int) (diff * mAudioState->audio_params_src.freq);
                    min_nb_samples = ((nbSamples * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100));
                    max_nb_samples = ((nbSamples * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100));
                    wanted_nb_samples = av_clip(wanted_nb_samples, min_nb_samples, max_nb_samples);
                }
            }
        } else {
            mAudioState->audio_diff_avg_count = 0;
            mAudioState->audio_diff_cum = 0;
        }
    }

    return wanted_nb_samples;
}

int AudioResampler::audioFrameResample() {
    int data_size, resampled_data_size;
    int64_t dec_channel_layout;
    int wanted_nb_samples;
    int translate_time = 1;
    int ret;
    // 处于暂停状态
    if (!mAudioDecoder || mPlayerState->abort_request || mPlayerState->pause_request) {
        return -1;
    }

    for (;;) {
        // 如果数据包解码失败，直接返回
        if ((ret = mAudioDecoder->getAudioFrame(mFrame)) < 0) { //获取解码得到的音频帧
            return -1;
        }
        if (ret == 0) {
            continue;
        }
        // 获取 frame 的大小
        data_size = av_samples_get_buffer_size(
                NULL,
                av_frame_get_channels(mFrame), mFrame->nb_samples,
                (AVSampleFormat) mFrame->format,
                1
        );
        // 音频布局
        dec_channel_layout = (mFrame->channel_layout && av_frame_get_channels(mFrame) == av_get_channel_layout_nb_channels(mFrame->channel_layout))
                             ? mFrame->channel_layout
                             : av_get_default_channel_layout(av_frame_get_channels(mFrame));

        wanted_nb_samples = audioSynchronize(mFrame->nb_samples);

        // 帧格式跟源格式不对？？？？当返回 frame 的格式跟音频原始参数不一样的时候，则修正
        if (mFrame->format != mAudioState->audio_params_src.fmt ||
            dec_channel_layout != mAudioState->audio_params_src.channel_layout ||
            mFrame->sample_rate != mAudioState->audio_params_src.freq ||
            (wanted_nb_samples != mFrame->nb_samples && !mAudioState->swr_ctx)) {

            swr_free(&mAudioState->swr_ctx);
            mAudioState->swr_ctx = swr_alloc_set_opts(
                    NULL,
                    mAudioState->audio_params_target.channel_layout,
                    mAudioState->audio_params_target.fmt,
                    mAudioState->audio_params_target.freq,
                    dec_channel_layout,
                    (AVSampleFormat) mFrame->format,
                    mFrame->sample_rate,
                    0,
                    NULL
            );

            if (!mAudioState->swr_ctx || swr_init(mAudioState->swr_ctx) < 0) {
                LOGE(
                        "AudioResampler->Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!",
                        mFrame->sample_rate,
                        av_get_sample_fmt_name((AVSampleFormat) mFrame->format),
                        av_frame_get_channels(mFrame),
                        mAudioState->audio_params_target.freq,
                        av_get_sample_fmt_name(mAudioState->audio_params_target.fmt),
                        mAudioState->audio_params_target.channels
                );
                swr_free(&mAudioState->swr_ctx);
                return -1;
            }
            mAudioState->audio_params_src.channel_layout = dec_channel_layout;
            mAudioState->audio_params_src.channels = av_frame_get_channels(mFrame);
            mAudioState->audio_params_src.freq = mFrame->sample_rate;
            mAudioState->audio_params_src.fmt = (AVSampleFormat) mFrame->format;
        }

        // 音频重采样处理
        if (mAudioState->swr_ctx) {
            const uint8_t **in = (const uint8_t **) mFrame->extended_data;
            uint8_t **out = &mAudioState->resample_buffer;
            int out_count = (int64_t) wanted_nb_samples * mAudioState->audio_params_target.freq / mFrame->sample_rate + 256;
            int out_size = av_samples_get_buffer_size(
                    NULL,
                    mAudioState->audio_params_target.channels,
                    out_count,
                    mAudioState->audio_params_target.fmt,
                    0
            );
            int len2;
            if (out_size < 0) {
                LOGE("AudioResampler->av_samples_get_buffer_size() failed");
                return -1;
            }
            if (wanted_nb_samples != mFrame->nb_samples) {
                // 激活重采样补偿（软补偿）
                if (swr_set_compensation(
                        mAudioState->swr_ctx,
                        (wanted_nb_samples - mFrame->nb_samples) * mAudioState->audio_params_target.freq /
                        mFrame->sample_rate,
                        wanted_nb_samples * mAudioState->audio_params_target.freq / mFrame->sample_rate
                ) < 0) {
                    LOGE("AudioResampler->swr_set_compensation() failed");
                    return -1;
                }
            }
            av_fast_malloc(&mAudioState->resample_buffer, &mAudioState->resample_size, out_size);
            if (!mAudioState->resample_buffer) {
                return AVERROR(ENOMEM);
            }

            /*
             * 针对每一帧音频的处理。把一帧帧的音频作相应的重采样
             * 参数1：音频重采样的上下文
             * 参数2：输出的指针。传递的输出的数组
             * 参数3：输出的样本数量，不是字节数。单通道的样本数量
             * 参数4：输入的数组，AVFrame 解码出来的 DATA
             * 参数5：输入的单通道的样本数量
             */
            len2 = swr_convert(mAudioState->swr_ctx, out, out_count, in, mFrame->nb_samples);
            if (len2 < 0) {
                LOGE("AudioResampler->swr_convert() failed");
                return -1;
            }
            if (len2 == out_count) {
                LOGW("AudioResampler->audio buffer is probably too small");
                if (swr_init(mAudioState->swr_ctx) < 0) {
                    swr_free(&mAudioState->swr_ctx);
                }
            }
            mAudioState->outputBuffer = mAudioState->resample_buffer;
            // 重采样得到的数据大小，单位 byte
            resampled_data_size = len2 * mAudioState->audio_params_target.channels *
                                  av_get_bytes_per_sample(mAudioState->audio_params_target.fmt);

            // 变速变调处理
            if ((mPlayerState->playback_rate != 1.0f || mPlayerState->playback_pitch != 1.0f) &&
                !mPlayerState->abort_request) {
                int bytes_per_sample = av_get_bytes_per_sample(mAudioState->audio_params_target.fmt);
                av_fast_malloc(
                        &mAudioState->sound_touch_buffer,
                        &mAudioState->sound_touch_buffer_size,
                        out_size * translate_time
                );
                for (int i = 0; i < (resampled_data_size / 2); i++) {
                    mAudioState->sound_touch_buffer[i] = (mAudioState->resample_buffer[i * 2] |
                                                          (mAudioState->resample_buffer[i * 2 + 1] << 8));
                }
                if (!mSoundTouchWrapper) {
                    mSoundTouchWrapper = new SoundTouchWrapper();
                }
                int ret_len = mSoundTouchWrapper->translate(
                        mAudioState->sound_touch_buffer,
                        (float) (mPlayerState->playback_rate),
                        (float) (mPlayerState->playback_pitch != 1.0f ? mPlayerState->playback_pitch : 1.0f / mPlayerState->playback_rate),
                        resampled_data_size / 2,
                        bytes_per_sample,
                        mAudioState->audio_params_target.channels,
                        mFrame->sample_rate
                );
                if (ret_len > 0) {
                    mAudioState->outputBuffer = (uint8_t *) mAudioState->sound_touch_buffer;
                    resampled_data_size = ret_len;
                } else {
                    translate_time++;
                    av_frame_unref(mFrame);
                    continue;
                }
            }
        } else {
            mAudioState->outputBuffer = mFrame->data[0];
            resampled_data_size = data_size;
        }

        // 处理完直接退出循环
        break;
    }

    // 利用 pts 更新音频时钟
    if (mFrame->pts != AV_NOPTS_VALUE) {
        mAudioState->audioClock = mFrame->pts * av_q2d((AVRational) {1, mFrame->sample_rate}) + (double) mFrame->nb_samples / mFrame->sample_rate;
    } else {
        mAudioState->audioClock = NAN;
    }

    // 使用完成释放引用，防止内存泄漏
    av_frame_unref(mFrame);

    return resampled_data_size;
}

