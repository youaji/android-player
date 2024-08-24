#ifndef FFMPEG4_VIDEODEVICE_H
#define FFMPEG4_VIDEODEVICE_H

#include <render/filter/input/header/GLInputFilter.h>
#include "PlayerState.h"

/**
 */
class VideoDevice {
public:
    /**
     */
    VideoDevice();

    /**
     */
    virtual ~VideoDevice();

    /**
     */
    virtual void terminate();

    /**
     * 设置时间戳
     * @param timeStamp
     */
    virtual void setTimeStamp(double timeStamp);

    /**
     * 初始化视频纹理宽高\n
     * 主要作用：初始化纹理相关，如果没有创建egl上下文和纹理渲染区域，就创建它们，并且关联egl上下文\n
     * 如果没有创建着色器和纹理对象，也要创建它们\n
     * 还要一些其他数据的初始化
     * @param width
     * @param height
     * @param format
     * @param blendMode
     * @param rotate
     */
    virtual void onInitTexture(int width, int height, TextureFormat format, BlendMode blendMode, int rotate = 0);

    /**
     * 更新YUV数据，分别更新YUV三个纹理对象的数据，数据分别有YUV的data数据和各自的宽度值
     * @param yData
     * @param yPitch
     * @param uData
     * @param uPitch
     * @param vData
     * @param vPitch
     * @return
     */
    virtual int onUpdateYUV(uint8_t *yData, int yPitch, uint8_t *uData, int uPitch, uint8_t *vData, int vPitch);

    /**
     * 更新 ARGB 数据
     * @param rgba
     * @param pitch
     * @return
     */
    virtual int onUpdateARGB(uint8_t *rgba, int pitch);

    /**
     * 请求渲染
     * @param flip
     * @return
     */
    virtual int onRequestRender(bool flip);

};

#endif //FFMPEG4_VIDEODEVICE_H
