#ifndef FFMPEG4_EGLHELPER_H
#define FFMPEG4_EGLHELPER_H

/**
 * Constructor flag: surface must be recordable.  This discourages EGL from using a
 * pixel format that cannot be converted efficiently to something usable by the video
 * encoder.
 */
#define FLAG_RECORDABLE 0x01

/**
 * Constructor flag: ask for GLES3, fall back to GLES2 if not available.  Without this
 * flag, GLES2 is used.
 */
#define FLAG_TRY_GLES3 002

#if defined(__ANDROID__)

#include <android/native_window.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>

// Android-specific extension
#define EGL_RECORDABLE_ANDROID 0x3142

// EGL functions should be prototyped as:
// EGLAPI return-type EGLAPIENTRY eglFunction(arguments);
// typedef return-type (EXPAPIENTRYP PFNEGLFUNCTIONPROC) (arguments);
typedef EGLBoolean (EGLAPIENTRYP EGL_PRESENTATION_TIME_ANDROIDPROC)(
        EGLDisplay display,
        EGLSurface surface,
        khronos_stime_nanoseconds_t time
);

#endif

class EglHelper {
public:
    EglHelper();

    virtual ~EglHelper();

    /**
     * 初始化
     * @param flags
     * @return
     */
    bool init(int flags);

    /**
     * 初始化 EGLDisplay、EGLContext、EGLConfig 等资源
     * @param sharedContext
     * @param flags
     * @return
     */
    bool init(EGLContext sharedContext, int flags);

    /**
     * 释放资源
     */
    void release();

    /**
     * @return 获取EglContext
     */
    EGLContext getEglContext();

    /**
     * 销毁Surface
     * @param eglSurface
     */
    void destroySurface(EGLSurface eglSurface);

    /**
     * 创建EGLSurface
     * @param surface
     * @return
     */
    EGLSurface createSurface(ANativeWindow *surface);

    /**
     * 创建离屏EGLSurface
     * @param width
     * @param height
     * @return
     */
    EGLSurface createSurface(int width, int height);

    /**
     * 切换到当前上下文
     * @param eglSurface
     */
    void makeCurrent(EGLSurface eglSurface);

    /**
     * 切换到某个上下文
     * @param drawSurface
     * @param readSurface
     */
    void makeCurrent(EGLSurface drawSurface, EGLSurface readSurface);

    /**
     * 没有上下文
     */
    void makeNothingCurrent();

    /**
     * 交换显示
     * @param eglSurface
     * @return
     */
    int swapBuffers(EGLSurface eglSurface);

    /**
     * 设置pts
     * @param eglSurface
     * @param nsecs
     */
    void setPresentationTime(EGLSurface eglSurface, long nsecs);

    /**
     * 判断是否属于当前上下文
     * @param eglSurface
     * @return
     */
    bool isCurrent(EGLSurface eglSurface);

    /**
     * 执行查询
     * @param eglSurface
     * @param what
     * @return
     */
    int querySurface(EGLSurface eglSurface, int what);

    /**
     * 查询字符串
     * @param what
     * @return
     */
    const char *queryString(int what);

    /**
     * 获取当前的GLES版本号
     * @return
     */
    int getGlVersion();

    /**
     * 检查是否出错
     * @param msg
     */
    void checkEglError(const char *msg);

private:
    /**
     * 查找合适的EGLConfig
     * @param flags
     * @param version
     * @return
     */
    EGLConfig getConfig(int flags, int version);

private:
    EGLDisplay mEglDisplay; //
    EGLConfig mEglConfig;   //
    EGLContext mEglContext; //
    int mGlVersion;         //

#if defined(__ANDROID__)
    // 设置时间戳方法，EGL_PRESENTATION_TIME_ANDROIDPROC 是一个方法的指针名字
    EGL_PRESENTATION_TIME_ANDROIDPROC eglPresentationTimeANDROID;
#endif
};

#endif //FFMPEG4_EGLHELPER_H
