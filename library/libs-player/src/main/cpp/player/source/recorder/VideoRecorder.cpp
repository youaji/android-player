#include "VideoRecorder.h"
#include "AndroidLog.h"

VideoRecorder::VideoRecorder(AVFormatContext *inputCtx, const char **filePath) {
    mInputCtx = inputCtx;
    mFilePath = (char *) *filePath;
    mRecorderThread = NULL;
    isRecording = false;
}

VideoRecorder::~VideoRecorder() {}

bool VideoRecorder::start() {
    LOGD("VideoRecorder->start[%s]", mFilePath);
////    stop();
//    int ret = avformat_alloc_output_context2(&mOutputCtx, NULL, "mpegts", mFilePath);
//    if (ret < 0) {
//        LOGE("VideoRecorder->Allocate an AVFormatContext for an output format failed,err=%d", ret);
//        closeOutput();
//        return false;
//    }
//
////    av_dump_format(mOutputCtx, 0, mFilePath, 1);
////    ret = avio_open(&mOutputCtx->pb, mFilePath, AVIO_FLAG_WRITE);
//    ret = avio_open2(&mOutputCtx->pb, mFilePath, AVIO_FLAG_WRITE, NULL, NULL);
//    if (ret < 0) {
//        LOGE("VideoRecorder->Create and initialize a AVIOContext failed,err=%d", ret);
//        closeOutput();
//        return false;
//    }
//
//    int nb_streams = mInputCtx->nb_streams;
//    for (int i = 0; i < nb_streams; i++) {
//        AVStream *stream = avformat_new_stream(mOutputCtx, mInputCtx->streams[i]->codec->codec);
//        ret = avcodec_copy_context(stream->codec, mInputCtx->streams[i]->codec);
//        if (ret < 0) {
//            LOGE("VideoRecorder->copy coddec context failed");
//        }
//    }
//
//    ret = avformat_write_header(mOutputCtx, NULL);
//    if (ret < 0) {
//        LOGE("VideoRecorder->write the stream header to an output media file failed,err=%d", ret);
//        closeOutput();
//        return false;
//    }
//
//    if (!mPacket) {
//        mPacket = (AVPacket *) av_malloc(sizeof(AVPacket));
//    }

    isRecording = true;
//    if (!mRecorderThread) {
//        mRecorderThread = new Thread(this);
//        mRecorderThread->start();
//        LOGD("VideoRecorder->开启视频录制线程");
//    }
//
//    return mRecorderThread->isActive();
    return true;
}

bool VideoRecorder::stop() {
    isRecording = false;
//    if (mRecorderThread) {
//        mRecorderThread->join();
//        delete mRecorderThread;
//        mRecorderThread = NULL;
//        LOGD("VideoRecorder->删除视频录制线程");
//    }
//    if (mPacket) {
//        av_packet_unref(mPacket);
//    }
    return true;
}

void VideoRecorder::closeOutput() {
    if (mOutputCtx) {
        for (int i = 0; i < mOutputCtx->nb_streams; i++) {
            AVStream *avStream = mOutputCtx->streams[i];
            if (avStream) {
                AVCodecContext *codecContext = avStream->codec;
                avcodec_close(codecContext);
            }
        }
        avformat_close_input(&mOutputCtx);
        avformat_free_context(mOutputCtx);
    }
}

int VideoRecorder::readPacketFromInput() {
    if (!mPacket) {
        LOGE("VideoRecorder->read a frame mPacket error");
        return -99;
    }

    av_init_packet(mPacket);
    if (!mInputCtx) {
        LOGE("VideoRecorder->read a frame mInputCtx error");
    }

    int ret = av_read_frame(mInputCtx, mPacket);

    if (ret < 0) {
        LOGE("VideoRecorder->read frame error or end of file");
        return ret;
    }
    LOGD("VideoRecorder->read a frame");
    return ret;
}

int VideoRecorder::writePacketToOutput() {
    int ret = -99;
    if (!mPacket) {
        LOGE("VideoRecorder->write a frame mPacket error");
        return ret;
    }

    AVStream *inputStream = mInputCtx->streams[mPacket->stream_index];
    AVStream *outputStream = mOutputCtx->streams[mPacket->stream_index];

//    //转换 PTS/DTS 时序
//    mPacket.pts = av_rescale_q_rnd(mPacket.pts, inputStream->time_base, outputStream->time_base, (enum AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
//    mPacket.dts = av_rescale_q_rnd(mPacket.dts, inputStream->time_base, outputStream->time_base, (enum AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
//    //printf("pts %d dts %d base %d\n",pkt.pts,pkt.dts, in_stream->time_base);
//    mPacket.duration = av_rescale_q(mPacket.duration, inputStream->time_base, outputStream->time_base);
//    mPacket.pos = -1;

    if (inputStream && outputStream) {
        // 处理同步
        av_packet_rescale_ts(mPacket, inputStream->time_base, outputStream->time_base);
        // 写入数据到输出文件
        ret = av_interleaved_write_frame(mOutputCtx, mPacket);
        if (ret < 0) {
            LOGE("VideoRecorder->write a packet to an output media file failed,err=%d", ret);
            return ret;
        }
    }
    LOGD("VideoRecorder->write a frame");
    return ret;
}

void VideoRecorder::run() {
    int writeRet;
    int readRet;
    LOGD("VideoRecorder->isRecording[%d]", isRecording);
    while (isRecording) {
        readRet = readPacketFromInput();
        if (readRet == 0) {
            writeRet = writePacketToOutput();
            LOGD("VideoRecorder->write a packet data ret[%d]", writeRet);
        } else {
            LOGE("VideoRecorder->read a packet data ret[%d]", readRet);
        }
    }
//    av_write_trailer(mOutputCtx);// 不知道为啥，这里可以不用？
    closeOutput();

    if (mPacket) {
        av_packet_unref(mPacket);
    }
    LOGD("VideoRecorder->save stream success[%s]", mFilePath);
}