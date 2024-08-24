#ifndef FFMPEG4_AUDIODECODER_H
#define FFMPEG4_AUDIODECODER_H

#include "MediaDecoder.h"
#include "PlayerState.h"

/**
 * 音频解码器
 */
class AudioDecoder : public MediaDecoder {
public:
    /**
     * @param avctx
     * @param stream
     * @param streamIndex
     * @param playerState
     */
    AudioDecoder(AVCodecContext *avctx,
                 AVStream *stream,
                 int streamIndex,
                 PlayerState *playerState);

    /**
     */
    virtual ~AudioDecoder();

    /**
     * @param frame
     * @return
     */
    int getAudioFrame(AVFrame *frame);

private:
    bool mPacketPending;     // 一次解码无法全部消耗完 AVPacket 中的数据的标志
    AVPacket *mPacket;       //
    int64_t mNextPTS;        //
    AVRational mNextPTS_tb;  //
};

#endif //FFMPEG4_AUDIODECODER_H
