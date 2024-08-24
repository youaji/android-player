#include "AudioDecoder.h"

AudioDecoder::AudioDecoder(AVCodecContext *avctx,
                           AVStream *stream,
                           int streamIndex,
                           PlayerState *playerState) : MediaDecoder(avctx, stream, streamIndex, playerState) {
    // 给数据包分配空间
    mPacket = av_packet_alloc();
    mPacketPending = 0;
}

AudioDecoder::~AudioDecoder() {
    mMutex.lock();
    mPacketPending = 0;
    if (mPacket) {
        av_packet_free(&mPacket);
        av_freep(&mPacket);
        mPacket = NULL;
    }
    mMutex.unlock();
}

int AudioDecoder::getAudioFrame(AVFrame *frame) {
    int got_frame = 0;
    int ret;

    if (!frame) {
        return AVERROR(ENOMEM);
    }

    av_frame_unref(frame);

    do {

        if (mAbortRequest) {
            ret = -1;
            break;
        }

        if (mPlayerState->seek_request) { // 正在定位
            continue;
        }

        AVPacket pkt;
        if (mPacketPending) {
            av_packet_move_ref(&pkt, mPacket);
            mPacketPending = 0;
        } else {
            // 取出数据包
            if (mPacketQueue->getPacket(&pkt) < 0) {
                ret = -1;
                break;
            }
        }

        mPlayerState->mutex.lock();
        // 将数据包解码
        ret = avcodec_send_packet(mAVCodecCtx, &pkt);
        if (ret < 0) {
            // 一次解码无法消耗完 AVPacket 中的所有数据，需要重新解码
            if (ret == AVERROR(EAGAIN)) {
                av_packet_move_ref(mPacket, &pkt);
                mPacketPending = 1;
            } else {
                av_packet_unref(&pkt);
                mPacketPending = 0;
            }
            mPlayerState->mutex.unlock();
            continue;
        }

        // 获取解码得到的音频帧 AVFrame
        ret = avcodec_receive_frame(mAVCodecCtx, frame);
        mPlayerState->mutex.unlock();
        // 释放数据包的引用，防止内存泄漏
        av_packet_unref(mPacket);
        if (ret < 0) {
            av_frame_unref(frame);
            got_frame = 0;
            continue;
        } else { // 取到了音频帧
            got_frame = 1;
            // 这里要重新计算 frame 的 pts
            // 否则会导致网络视频出现 pts 对不上的情况
            AVRational tb = (AVRational) {1, frame->sample_rate};
            if (frame->pts != AV_NOPTS_VALUE) {
                // 在 MediaPlayer 中有 av_codec_set_pkt_timebase 设置时间基的操作
                // 把 pts 的值从基于第一个时间基调整到基于第二个时间基
                frame->pts = av_rescale_q(frame->pts, av_codec_get_pkt_timebase(mAVCodecCtx), tb);
            } else if (mNextPTS != AV_NOPTS_VALUE) {
                frame->pts = av_rescale_q(mNextPTS, mNextPTS_tb, tb);
            }
            if (frame->pts != AV_NOPTS_VALUE) {
                mNextPTS = frame->pts + frame->nb_samples;
                mNextPTS_tb = tb;
            }
        }
    } while (!got_frame); // 解码帧成功即退出

    if (ret < 0) {
        return -1;
    }

    return got_frame;
}
