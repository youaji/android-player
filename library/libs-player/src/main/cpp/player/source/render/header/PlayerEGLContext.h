#ifndef FFMPEG4_PLAYEREGLCONTEXT_H
#define FFMPEG4_PLAYEREGLCONTEXT_H

#include <mutex>

#if defined(__ANDROID__)

#include <android/native_window.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>

#endif

class PlayerEGLContext {

public:
    static PlayerEGLContext *getInstance();

    void destory();

    EGLContext getContext();

private:
    PlayerEGLContext();

    virtual ~PlayerEGLContext();

    bool init(int flags);

    void release();

    EGLConfig getConfig(int flags, int version);

    void checkEglError(const char *msg);

    static PlayerEGLContext *instance;
    static std::mutex mutex;

    EGLContext eglContext;
    EGLDisplay eglDisplay;
};

#endif //FFMPEG4_PLAYEREGLCONTEXT_H
