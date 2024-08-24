#ifndef GLBEAUTYADJUSTFILTER_H
#define GLBEAUTYADJUSTFILTER_H

#include <GLFilter.h>

/**
 * 磨皮调节滤镜，高反差保留法最后一步
 */
class GLBeautyAdjustFilter : public GLFilter {
public:
    GLBeautyAdjustFilter();

    virtual ~GLBeautyAdjustFilter();

    void initProgram() override;

    void initProgram(const char *vertexShader, const char *fragmentShader) override;

    void setBlurTexture(int blurTexture, int highPassBlurTexture);

protected:
    void onDrawBegin() override;

    int mBlurTextureHandle;          // 第一轮高斯模糊纹理句柄
    int mHighPassBlurTextureHandle;  // 第二轮高斯模糊纹理句柄
    int mIntensityHandle;            // 强度句柄

    int mBlurTexture;
    int mHighPassBlurTexture;
};

#endif //GLBEAUTYADJUSTFILTER_H