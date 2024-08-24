#include "YouajiMediaPlayer.h"
#include <AVMessageQueue.h>

YouajiMediaPlayer::YouajiMediaPlayer() {
    mMessageThread = nullptr;
    mIsAbortRequest = true;
    mVideoDevice = nullptr;
    mMediaPlayer = nullptr;
    mMediaPlayerListener = nullptr;
    mIsPrepareSync = false;
    mPrepareStatus = NO_ERROR;
    mAudioSessionId = 0;
    mIsSeeking = false;
    mSeekingPosition = 0;
}


YouajiMediaPlayer::~YouajiMediaPlayer() {
}

// 对应于java层的 MediaPlayer 创建对象的时候调用，即在构造函数中被调用
void YouajiMediaPlayer::init() {
    mMutex.lock();
    mIsAbortRequest = false;
    mCondition.signal();
    mMutex.unlock();

    mMutex.lock();

    // 视频播放设备
    if (mVideoDevice == nullptr) {
        mVideoDevice = new GLESDevice();
    }

    // 消息分发的线程
    if (mMessageThread == nullptr) {
        mMessageThread = new Thread(this);
        mMessageThread->start();
    }
    mMutex.unlock();
}

void YouajiMediaPlayer::setWatermark(uint8_t *watermarkPixel, size_t length, GLint width, GLint height, GLfloat scale, GLint location) {
    if (mVideoDevice != nullptr) {
        mVideoDevice->setWatermark(watermarkPixel, length, width, height, scale, location);
    }
}

void YouajiMediaPlayer::changeFilter(int type, const char *name) {
    if (mVideoDevice != nullptr) {
        mVideoDevice->changeFilter((RenderNodeType) type, name);
    }
}

void YouajiMediaPlayer::changeFilter(int type, const int id) {
    if (mVideoDevice != nullptr) {
        mVideoDevice->changeFilter((RenderNodeType) type, id);
    }
}


// java层对应 MediaPlayer 的 release 方法的时候被调用
void YouajiMediaPlayer::disconnect() {
    mMutex.lock();
    mIsAbortRequest = true;
    mCondition.signal();
    mMutex.unlock();
    // 重置
    reset();
    LOGD("YouajiMediaPlayer->player disconnect");
    if (mMessageThread != nullptr) {
        LOGD("YouajiMediaPlayer->删除消息通知线程---开始");
        mMessageThread->join();
        delete mMessageThread;
        LOGD("YouajiMediaPlayer->删除消息通知线程---完成");
        mMessageThread = nullptr;
    }

    if (mVideoDevice != nullptr) {
        delete mVideoDevice;
        mVideoDevice = nullptr;
    }
    if (mMediaPlayerListener != nullptr) {
        delete mMediaPlayerListener;
        mMediaPlayerListener = nullptr;
    }
}

status_t YouajiMediaPlayer::setDataSource(const char *url, int64_t offset, const char *headers) {
    if (url == nullptr) {
        return BAD_VALUE;
    }

    if (mMediaPlayer == nullptr) {
        mMediaPlayer = new MediaPlayer();
    }
    mMediaPlayer->setDataSource(url, offset, headers);
    mMediaPlayer->setVideoDevice(mVideoDevice);
    return NO_ERROR;
}

status_t YouajiMediaPlayer::setMetadataFilter(char **allow, char **block) {
    // do nothing
    return NO_ERROR;
}

status_t YouajiMediaPlayer::getMetadata(bool update_only, bool apply_filter, AVDictionary **metadata) {
    if (mMediaPlayer != nullptr) {
        return mMediaPlayer->getMetadata(metadata);
    }
    return NO_ERROR;
}

void YouajiMediaPlayer::surfaceChanged(int width, int height) {
    if (mVideoDevice != nullptr) {
        mVideoDevice->surfaceChanged(width, height);
    }
}

status_t YouajiMediaPlayer::setVideoSurface(ANativeWindow *native_window) {
    if (mMediaPlayer == nullptr) {
        return NO_INIT;
    }
    if (native_window != nullptr) {
        mVideoDevice->surfaceCreated(native_window);
        return NO_ERROR;
    }
    return NO_ERROR;
}


status_t YouajiMediaPlayer::setListener(MediaPlayerListener *listener) {
    if (mMediaPlayerListener != nullptr) {
        delete mMediaPlayerListener;
    }
    mMediaPlayerListener = listener;
    return NO_ERROR;
}

status_t YouajiMediaPlayer::prepare() {
    if (mMediaPlayer == nullptr) {
        return NO_INIT;
    }
    if (mIsPrepareSync) {
        return -EALREADY;
    }
    mIsPrepareSync = true;
    status_t ret = mMediaPlayer->prepare();
    if (ret != NO_ERROR) {
        return ret;
    }
    if (mIsPrepareSync) {
        mIsPrepareSync = false;
    }
    return mPrepareStatus;
}

status_t YouajiMediaPlayer::prepareAsync() {
    if (mMediaPlayer != nullptr) {
        return mMediaPlayer->prepareAsync();
    }
    return INVALID_OPERATION;
}

void YouajiMediaPlayer::start() {
    if (mMediaPlayer != nullptr) {
        mMediaPlayer->start();
    }
}

void YouajiMediaPlayer::stop() {
    if (mMediaPlayer) {
        mMediaPlayer->stop();
    }
}

void YouajiMediaPlayer::pause() {
    if (mMediaPlayer) {
        mMediaPlayer->pause();
    }
}

void YouajiMediaPlayer::resume() {
    if (mMediaPlayer) {
        mMediaPlayer->resume();
    }
}

bool YouajiMediaPlayer::isPlaying() {
    if (mMediaPlayer) {
        return (mMediaPlayer->isPlaying() != 0);
    }
    return false;
}

int YouajiMediaPlayer::getRotate() {
    if (mMediaPlayer != nullptr) {
        return mMediaPlayer->getRotate();
    }
    return 0;
}

int YouajiMediaPlayer::getVideoWidth() {
    if (mMediaPlayer != nullptr) {
        return mMediaPlayer->getVideoWidth();
    }
    return 0;
}

int YouajiMediaPlayer::getVideoHeight() {
    if (mMediaPlayer != nullptr) {
        return mMediaPlayer->getVideoHeight();
    }
    return 0;
}

status_t YouajiMediaPlayer::seekTo(float msec) {
    if (mMediaPlayer != nullptr) {
        // if in seeking state, put seek message in queue, to process after preview seeking.
        if (mIsSeeking) {
            mMediaPlayer->getMessageQueue()->postMessage(MSG_REQUEST_SEEK, msec);
        } else {
            mMediaPlayer->seekTo(msec);
            mSeekingPosition = (long) msec;
            mIsSeeking = true;
        }
    }
    return NO_ERROR;
}

long YouajiMediaPlayer::getCurrentPosition() {
    if (mMediaPlayer != nullptr) {
        if (mIsSeeking) {
            return mSeekingPosition;
        }
        return mMediaPlayer->getCurrentPosition();
    }
    return 0;
}

long YouajiMediaPlayer::getDuration() {
    if (mMediaPlayer != nullptr) {
        return mMediaPlayer->getDuration();
    }
    return -1;
}

status_t YouajiMediaPlayer::reset() {
    mIsPrepareSync = false;
    if (mMediaPlayer != nullptr) {
        mMediaPlayer->reset();
        delete mMediaPlayer;
        mMediaPlayer = nullptr;
    }
    return NO_ERROR;
}

status_t YouajiMediaPlayer::setAudioStreamType(int type) {
    return NO_ERROR;
}

status_t YouajiMediaPlayer::setLooping(bool looping) {
    if (mMediaPlayer != nullptr) {
        mMediaPlayer->setLooping(looping);
    }
    return NO_ERROR;
}

bool YouajiMediaPlayer::isLooping() {
    if (mMediaPlayer != nullptr) {
        return (mMediaPlayer->isLooping() != 0);
    }
    return false;
}

status_t YouajiMediaPlayer::setVolume(float volume) {
    if (mMediaPlayer != nullptr) {
        mMediaPlayer->setVolume(volume);
    }
    return NO_ERROR;
}

float YouajiMediaPlayer::getVolume() {
    if (mMediaPlayer != nullptr) {
        return mMediaPlayer->getVolume();
    }
    return 0;
}

status_t YouajiMediaPlayer::setStereoVolume(float leftVolume, float rightVolume) {
    if (mMediaPlayer != nullptr) {
        mMediaPlayer->setStereoVolume(leftVolume, rightVolume);
    }
    return NO_ERROR;
}

void YouajiMediaPlayer::setMute(bool mute) {
    if (mMediaPlayer != nullptr) {
        mMediaPlayer->setMute(mute);
    }
}

void YouajiMediaPlayer::setRate(float speed) {
    if (mMediaPlayer != nullptr) {
        mMediaPlayer->setRate(speed);
    }
}

void YouajiMediaPlayer::setPitch(float pitch) {
    if (mMediaPlayer != nullptr) {
        mMediaPlayer->setPitch(pitch);
    }
}

status_t YouajiMediaPlayer::setAudioSessionId(int sessionId) {
    if (sessionId < 0) {
        return BAD_VALUE;
    }
    mAudioSessionId = sessionId;
    return NO_ERROR;
}

int YouajiMediaPlayer::getAudioSessionId() {
    return mAudioSessionId;
}

void YouajiMediaPlayer::setOption(int category, const char *type, const char *option) {
    if (mMediaPlayer != nullptr) {
        mMediaPlayer->getPlayerState()->setOption(category, type, option);
    }
}

void YouajiMediaPlayer::setOption(int category, const char *type, int64_t option) {
    if (mMediaPlayer != nullptr) {
        mMediaPlayer->getPlayerState()->setOptionLong(category, type, option);
    }
}

void YouajiMediaPlayer::notify(int msg, int ext1, int ext2, void *obj, int len) {
    if (mMediaPlayer != nullptr) {
        mMediaPlayer->getMessageQueue()->postMessage(msg, ext1, ext2, obj, len);
    }
}

bool YouajiMediaPlayer::startRecord(const char *filePath) {
    if (mMediaPlayer != nullptr) {
        return (mMediaPlayer->startRecord(filePath) != 0);
    }
    return false;
}


bool YouajiMediaPlayer::stopRecord() {
    if (mMediaPlayer != nullptr) {
        return (mMediaPlayer->stopRecord() != 0);
    }
    return false;
}

bool YouajiMediaPlayer::isRecording() {
    if (mMediaPlayer != nullptr) {
        return (mMediaPlayer->isRecording() != 0);
    }
    return false;
}

bool YouajiMediaPlayer::screenshot(const char *filePath) {
    if (mMediaPlayer != nullptr) {
        return (mMediaPlayer->screenshot(filePath) != 0);
    }
    return false;
}

void YouajiMediaPlayer::postEvent(int what, int arg1, int arg2, void *obj) {
    string str;

    if (mMediaPlayerListener != nullptr) {
        mMediaPlayerListener->notify(what, arg1, arg2, obj);
    }
}

/**
 * 这个线程用来循环获取消息队列中的消息，
 * 然后通知出去，可以在java层接收到消息的通知，
 * 主要是播放状态之类的消息
 */
void YouajiMediaPlayer::run() {

    int retval;
    while (true) {

        if (mIsAbortRequest) {
            break;
        }

        // 如果此时播放器还没准备好，则睡眠10毫秒，等待播放器初始化
        if (!mMediaPlayer || !mMediaPlayer->getMessageQueue()) {
            av_usleep(10 * 1000);
            continue;
        }

        AVMessage msg;
        retval = mMediaPlayer->getMessageQueue()->getMessage(&msg);
        if (retval < 0) {
            LOGE("YouajiMediaPlayer->player get message error.");
            break;
        }
        assert(retval > 0);

        switch (msg.what) {
            case MSG_FLUSH: {
                LOGD("YouajiMediaPlayer->[POST EVENT] flushing.");
                postEvent(MEDIA_NOP, 0, 0);
                break;
            }

            case MSG_ERROR: {
                LOGE("YouajiMediaPlayer->[POST EVENT] occurs error %d.", msg.arg1);
                if (mIsPrepareSync) {
                    mIsPrepareSync = false;
                    mPrepareStatus = msg.arg1;
                }
                postEvent(MEDIA_ERROR, msg.arg1, 0);
                break;
            }

            case MSG_PREPARED: {
                LOGD("YouajiMediaPlayer->[POST EVENT] prepared.");
                if (mIsPrepareSync) {
                    mIsPrepareSync = false;
                    mPrepareStatus = NO_ERROR;
                }
                postEvent(MEDIA_PREPARED, 0, 0);
                break;
            }

            case MSG_STARTED: {
                LOGD("YouajiMediaPlayer->[POST EVENT] started.");
                postEvent(MEDIA_STARTED, 0, 0);
                break;
            }

            case MSG_COMPLETED: {
                LOGD("YouajiMediaPlayer->[POST EVENT] playback completed.");
                postEvent(MEDIA_PLAYBACK_COMPLETE, 0, 0);
                break;
            }

            case MSG_VIDEO_SIZE_CHANGED: {
                LOGD("YouajiMediaPlayer->[POST EVENT] video size changing %d x %d.", msg.arg1, msg.arg2);
                postEvent(MEDIA_SET_VIDEO_SIZE, msg.arg1, msg.arg2);
                break;
            }

            case MSG_SAR_CHANGED: {
                LOGD("YouajiMediaPlayer->[POST EVENT] sar changing %d, %d.", msg.arg1, msg.arg2);
                postEvent(MEDIA_SET_VIDEO_SAR, msg.arg1, msg.arg2);
                break;
            }

            case MSG_VIDEO_RENDERING_START: {
                LOGD("YouajiMediaPlayer->[POST EVENT] video playing.");
                break;
            }

            case MSG_AUDIO_RENDERING_START: {
                LOGD("YouajiMediaPlayer->[POST EVENT] audio playing.");
                break;
            }

            case MSG_VIDEO_ROTATION_CHANGED: {
                LOGD("YouajiMediaPlayer->[POST EVENT] video rotation changing %d.", msg.arg1);
                break;
            }

            case MSG_AUDIO_START: {
                LOGD("YouajiMediaPlayer->[POST EVENT] start audio decoder.");
                break;
            }

            case MSG_VIDEO_START: {
                LOGD("YouajiMediaPlayer->[POST EVENT] start video decoder.");
                break;
            }

            case MSG_OPEN_INPUT: {
                LOGD("YouajiMediaPlayer->[POST EVENT] opening input file.");
                break;
            }

            case MSG_FIND_STREAM_INFO: {
                LOGD("YouajiMediaPlayer->[POST EVENT] finding media stream info.");
                break;
            }

            case MSG_BUFFERING_START: {
                LOGD("YouajiMediaPlayer->[POST EVENT] buffering start.");
                postEvent(MEDIA_INFO, MEDIA_INFO_BUFFERING_START, msg.arg1);
                break;
            }

            case MSG_BUFFERING_END: {
                LOGD("YouajiMediaPlayer->[POST EVENT] buffering finish.");
                postEvent(MEDIA_INFO, MEDIA_INFO_BUFFERING_END, msg.arg1);
                break;
            }

            case MSG_BUFFERING_UPDATE: {
                LOGD("YouajiMediaPlayer->[POST EVENT] buffering %d %d.", msg.arg1, msg.arg2);
                postEvent(MEDIA_BUFFERING_UPDATE, msg.arg1, msg.arg2);
                break;
            }

            case MSG_BUFFERING_TIME_UPDATE: {
                LOGD("YouajiMediaPlayer->[POST EVENT] time text update");
                break;
            }

            case MSG_SEEK_COMPLETE: {
                LOGD("YouajiMediaPlayer->[POST EVENT] seeks completed.");
                mIsSeeking = false;
                postEvent(MEDIA_SEEK_COMPLETE, 0, 0);
                break;
            }

            case MSG_PLAYBACK_STATE_CHANGED: {
                LOGD("YouajiMediaPlayer->[POST EVENT] playback state changed.");
                break;
            }

            case MSG_TIMED_TEXT: {
                LOGD("YouajiMediaPlayer->[POST EVENT] updating time text.");
                postEvent(MEDIA_TIMED_TEXT, 0, 0, msg.obj);
                break;
            }

            case MSG_REQUEST_PREPARE: {
                LOGD("YouajiMediaPlayer->[POST EVENT] preparing...");
                status_t ret = prepare();
                if (ret != NO_ERROR) {
                    LOGE("YouajiMediaPlayer->player prepare error %d.", ret);
                }
                break;
            }

            case MSG_REQUEST_START: {
                LOGD("YouajiMediaPlayer->[POST EVENT] waiting to start.");
                break;
            }

            case MSG_REQUEST_PAUSE: {
                LOGD("YouajiMediaPlayer->[POST EVENT] pausing...");
                pause();
                break;
            }

            case MSG_REQUEST_SEEK: {
                LOGD("YouajiMediaPlayer->[POST EVENT] seeking...");
                mIsSeeking = true;
                mSeekingPosition = (long) msg.arg1;
                if (mMediaPlayer != nullptr) {
                    mMediaPlayer->seekTo(mSeekingPosition);
                }
                break;
            }

            case MSG_CURRENT_POSITION: {
//                LOGD("YouajiMediaPlayer->[POST EVENT] current position %d %d.", msg.arg1, msg.arg2);
                postEvent(MEDIA_CURRENT, msg.arg1, msg.arg2);
                break;
            }

            default: {
                LOGE("YouajiMediaPlayer->[POST EVENT] unknown [what:%d, arg1:%d, arg2:%d].", msg.what, msg.arg1, msg.arg2);
                break;
            }
        }
        message_free_resouce(&msg);
    }
}