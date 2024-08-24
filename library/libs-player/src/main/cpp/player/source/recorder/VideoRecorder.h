#ifndef FFMPEG4_VIDEORECORDER_H
#define FFMPEG4_VIDEORECORDER_H


#include "PlayerState.h"

class VideoRecorder : public Runnable {

private:
    Mutex mMutex;
    Thread *mRecorderThread;
    AVFormatContext *mInputCtx;
    AVFormatContext *mOutputCtx;
    AVPacket *mPacket;
    char *mFilePath;

    void closeOutput();

    int readPacketFromInput();

    int writePacketToOutput();

protected:
    void run() override;

public:
    bool isRecording = false;

    VideoRecorder(AVFormatContext *inputCtx, const char **filePath);

    virtual ~VideoRecorder();

    bool start();

    bool stop();
};

#endif //FFMPEG4_VIDEORECORDER_H
