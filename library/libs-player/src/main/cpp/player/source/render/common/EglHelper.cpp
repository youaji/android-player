#include "render/common/header/EglHelper.h"
#include "AndroidLog.h"
#include "PlayerEGLContext.h"

EglHelper::EglHelper() {
    mEglDisplay = EGL_NO_DISPLAY;
    mEglConfig = NULL;
    mEglContext = EGL_NO_CONTEXT;
    mGlVersion = -1;

#if defined(__ANDROID__)
    // 设置时间戳方法，只有Android存在这个方法
    eglPresentationTimeANDROID = NULL;
#endif
}

EglHelper::~EglHelper() {
    release();
}

bool EglHelper::init(int flags) {
    // 主要初始化 eglPlayer 和 EGLContext
    return init(PlayerEGLContext::getInstance()->getContext(), flags);
}

bool EglHelper::init(EGLContext sharedContext, int flags) {
    if (mEglDisplay != EGL_NO_DISPLAY) {
        LOGE("EglHelper->EGL already set up");
        return false;
    }

    if (sharedContext == NULL) {
        LOGD("EglHelper->Shared Context is null");
        sharedContext = EGL_NO_CONTEXT;
    } else {
        LOGD("EglHelper->Main EGLContext is created!");
    }

    // 获取EGLDisplay
    mEglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (mEglDisplay == EGL_NO_DISPLAY) {
        LOGE("EglHelper->unable to get EGLDisplay.");
        return false;
    }

    // 初始化mEGLDisplay
    if (!eglInitialize(mEglDisplay, 0, 0)) {
        mEglDisplay = EGL_NO_DISPLAY;
        LOGE("EglHelper->unable to initialize EGLDisplay.");
        return false;
    }

    // 判断是否尝试使用GLES3
    if ((flags & FLAG_TRY_GLES3) != 0) {
        EGLConfig config = getConfig(flags, 3);
        if (config != NULL) {
            int attrib3_list[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
            // sharedContext：允许其它 EGLContext 共享数据，使用 EGL_NO_CONTEXT 表示不共享
            EGLContext context = eglCreateContext(mEglDisplay, config, sharedContext, attrib3_list);
            checkEglError("eglCreateContext");
            if (eglGetError() == EGL_SUCCESS) {
                mEglConfig = config;
                mEglContext = context;
                mGlVersion = 3;
            }
        }
    }

    // 判断如果GLES3的EGLContext没有获取到，则尝试使用GLES2
    if (mEglContext == EGL_NO_CONTEXT) {
        EGLConfig config = getConfig(flags, 2);
        int attrib2_list[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
        EGLContext context = eglCreateContext(mEglDisplay, config, sharedContext, attrib2_list);
        checkEglError("eglCreateContext");
        if (eglGetError() == EGL_SUCCESS) {
            mEglConfig = config;
            mEglContext = context;
            mGlVersion = 2;
        }
    }

#if defined(__ANDROID__)
    // 获取eglPresentationTimeANDROID方法的地址，读取动态库中eglGetProcAddress函数指针
    // eglGetProcAddress返回的是egl函数eglPresentationTimeANDROID的指针，
    eglPresentationTimeANDROID = (EGL_PRESENTATION_TIME_ANDROIDPROC) eglGetProcAddress("eglPresentationTimeANDROID");
    if (!eglPresentationTimeANDROID) {
        LOGE("EglHelper->eglPresentationTimeANDROID is not available!");
    }
#endif

    int values[1] = {0};
    eglQueryContext(mEglDisplay, mEglContext, EGL_CONTEXT_CLIENT_VERSION, values);
    LOGD("EglHelper->EGLContext created, client version %d", values[0]);
    return true;
}

void EglHelper::release() {
    if (mEglDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }
    if (mEglContext != EGL_NO_CONTEXT) {
        eglDestroyContext(mEglDisplay, mEglContext);
    }
    if (mEglDisplay != EGL_NO_DISPLAY) {
        eglReleaseThread();
        eglTerminate(mEglDisplay);
    }
    mEglDisplay = EGL_NO_DISPLAY;
    mEglConfig = NULL;
    mEglContext = EGL_NO_CONTEXT;
}

EGLContext EglHelper::getEglContext() {
    return mEglContext;
}

void EglHelper::destroySurface(EGLSurface eglSurface) {
    eglDestroySurface(mEglDisplay, eglSurface);
}

EGLSurface EglHelper::createSurface(ANativeWindow *surface) {
    if (surface == NULL) {
        LOGE("EglHelper->Window surface is NULL!");
        return NULL;
    }
    int attrib_list[] = {
            EGL_NONE
    };
    EGLSurface eglSurface = eglCreateWindowSurface(mEglDisplay, mEglConfig, surface, attrib_list);
    checkEglError("eglCreateWindowSurface");
    if (eglSurface == EGL_NO_SURFACE) {
        LOGE("EglHelper->EGLSurface was null");
    }
    return eglSurface;
}

EGLSurface EglHelper::createSurface(int width, int height) {
    int attrib_list[] = {
            EGL_WIDTH, width,
            EGL_HEIGHT, height,
            EGL_NONE
    };
    EGLSurface eglSurface = eglCreatePbufferSurface(mEglDisplay, mEglConfig, attrib_list);
    checkEglError("eglCreatePbufferSurface"); //检查错误
    if (eglSurface == EGL_NO_SURFACE) {
        LOGE("EGLSurface was null");
    }
    return eglSurface;
}

void EglHelper::makeCurrent(EGLSurface eglSurface) {
    if (mEglDisplay == EGL_NO_DISPLAY) {
        LOGD("EglHelper->Note: makeCurrent w/o display.");
    }

    if (!eglMakeCurrent(mEglDisplay, eglSurface, eglSurface, mEglContext)) {
        // 出错处理
    }
}

void EglHelper::makeCurrent(EGLSurface drawSurface, EGLSurface readSurface) {
    if (mEglDisplay == EGL_NO_DISPLAY) {
        LOGD("EglHelper->Note: makeCurrent w/o display.");
    }
    if (!eglMakeCurrent(mEglDisplay, drawSurface, readSurface, mEglContext)) {
        // 出错处理
    }
}

void EglHelper::makeNothingCurrent() {
    if (!eglMakeCurrent(mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)) {
        // 出错处理
    }
}

int EglHelper::swapBuffers(EGLSurface eglSurface) {
    if (!eglSwapBuffers(mEglDisplay, eglSurface)) {
        return eglGetError();
    }
    return EGL_SUCCESS;
}

#if defined(__ANDROID__) //如果是Android系统

void EglHelper::setPresentationTime(EGLSurface eglSurface, long nsecs) {
    if (eglPresentationTimeANDROID != NULL) {
        eglPresentationTimeANDROID(mEglDisplay, eglSurface, nsecs);
    }
}

#endif

bool EglHelper::isCurrent(EGLSurface eglSurface) {
    return (mEglContext == eglGetCurrentContext()) && (eglSurface == eglGetCurrentSurface(EGL_DRAW));
}

int EglHelper::querySurface(EGLSurface eglSurface, int what) {
    int value;
    eglQuerySurface(mEglContext, eglSurface, what, &value);
    return value;
}

const char *EglHelper::queryString(int what) {
    return eglQueryString(mEglDisplay, what);
}

int EglHelper::getGlVersion() {
    return mGlVersion;
}

void EglHelper::checkEglError(const char *msg) {
    int error;
    if ((error = eglGetError()) != EGL_SUCCESS) {
        LOGE("EglHelper->%s: EGL error: %x", msg, error);
    }
}

EGLConfig EglHelper::getConfig(int flags, int version) {
    int renderableType = EGL_OPENGL_ES2_BIT;
    if (version >= 3) {
        renderableType |= EGL_OPENGL_ES3_BIT_KHR;
    }
    int attribList[] = {
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            //EGL_DEPTH_SIZE, 16,
            //EGL_STENCIL_SIZE, 8,
            EGL_RENDERABLE_TYPE, renderableType,
            EGL_NONE, 0,      // placeholder for recordable [@-3]
            EGL_NONE
    };
    // attribList的长度
    int length = sizeof(attribList) / sizeof(attribList[0]);
    if ((flags & FLAG_RECORDABLE) != 0) {
        attribList[length - 3] = EGL_RECORDABLE_ANDROID;
        attribList[length - 2] = 1;
    }
    EGLConfig configs = NULL;
    int numConfigs;
    if (!eglChooseConfig(mEglDisplay, attribList, &configs, 1, &numConfigs)) {
        LOGW("EglHelper->unable to find RGB8888 / %d  EGLConfig", version);
        return NULL;
    }
    return configs;
}
