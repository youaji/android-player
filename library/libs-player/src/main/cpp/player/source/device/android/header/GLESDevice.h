#ifndef FFMPEG4_GLESDEVICE_H
#define FFMPEG4_GLESDEVICE_H

#include "VideoDevice.h"
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <Filter.h>
#include <RenderNodeList.h>
#include "render/common/header/EglHelper.h"
#include "InputRenderNode.h"

/**
 */
class GLESDevice final : public VideoDevice {
public:
    GLESDevice();

    virtual ~GLESDevice();

    /**
     * @param window
     */
    void surfaceCreated(ANativeWindow *window);

    /**
     * Surface 的大小发生改变
     * @param width
     * @param height
     */
    void surfaceChanged(int width, int height);

    /**
     * 改变滤镜
     * @param type
     * @param filterName
     */
    void changeFilter(RenderNodeType type, const char *filterName);

    /**
     * 改变滤镜
     * @param type
     * @param id
     */
    void changeFilter(RenderNodeType type, const int id);

    /**
     * 设置水印
     * @param watermarkPixel
     * @param length
     * @param width
     * @param height
     * @param scale
     * @param location
     */
    void setWatermark(uint8_t *watermarkPixel, size_t length, GLint width, GLint height, GLfloat scale, GLint location);

    /**
     * @param releaseContext
     */
    void terminate(bool releaseContext);

    void terminate() override;

    void setTimeStamp(double timeStamp) override;

    void onInitTexture(int width, int height, TextureFormat format, BlendMode blendMode, int rotate) override;

    int onUpdateYUV(uint8_t *yData, int yPitch, uint8_t *uData, int uPitch, uint8_t *vData, int vPitch) override;

    int onUpdateARGB(uint8_t *rgba, int pitch) override;

    int onRequestRender(bool flip) override;

private:
    void resetVertices();

    void resetTextureVertices();

private:
    Mutex mMutex;
    Condition mCondition;

    ANativeWindow *mNativeWindow;       // Surface 窗口
    int mSurfaceWidth;                  // 窗口宽度
    int mSurfaceHeight;                 // 窗口高度
    EGLSurface mEGLSurface;             // eglSurface
    EglHelper *mEglHelper;              // EGL帮助器
    bool mIsSurfaceReset;               // 重新设置 Surface
    bool mIsHasSurface;                 // 是否存在 Surface
    bool mIsHasEGLSurface;              // EGLSurface
    bool mIsHasEGlContext;              // 释放资源

    Texture *mVideoTexture;             // 视频纹理
    InputRenderNode *mInputRenderNode;  // 输入渲染结点
    float mVertices[8];                 // 顶点坐标
    float mTextureVertices[8];          // 纹理坐标

    RenderNodeList *mNodeList;          // 滤镜链
    FilterInfo mFilterInfo;             // 滤镜信息
    bool mIsFilterChange;               // 切换滤镜


};

#endif //FFMPEG4_GLESDEVICE_H
