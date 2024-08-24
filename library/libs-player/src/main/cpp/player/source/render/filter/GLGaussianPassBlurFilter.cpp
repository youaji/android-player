
#include <AndroidLog.h>
#include "GLGaussianPassBlurFilter.h"

const std::string kGaussianPassVertexShader = SHADER_TO_STRING(

        attribute vec4 aPosition;
        attribute vec4 aTextureCoord;

        // 高斯算子左右偏移值，当偏移值为2时，高斯算子为5 x 5
        const int SHIFT_SIZE = 2;

        uniform highp float texelWidthOffset;
        uniform highp float texelHeightOffset;

        varying vec2 textureCoordinate;
        varying vec4 blurShiftCoordinates[SHIFT_SIZE];

        void main() {
            gl_Position = aPosition;
            textureCoordinate = aTextureCoord.xy;
            // 偏移步距
            vec2 singleStepOffset = vec2(texelWidthOffset, texelHeightOffset);
            // 记录偏移坐标
            for (int i = 0; i < SHIFT_SIZE; i++) {
                blurShiftCoordinates[i] = vec4(textureCoordinate.xy - float(i + 1) * singleStepOffset,
                                               textureCoordinate.xy + float(i + 1) * singleStepOffset);
            }
        }
);

const std::string kGaussianPassFragmentShader = SHADER_TO_STRING(

        precision mediump float;
        varying vec2 textureCoordinate;
        uniform sampler2D inputTexture;
        // 高斯算子左右偏移值，当偏移值为2时，高斯算子为5 x 5
        const int SHIFT_SIZE = 2;
        varying vec4 blurShiftCoordinates[SHIFT_SIZE];
        void main() {
            // 计算当前坐标的颜色值
            vec4 currentColor = texture2D(inputTexture, textureCoordinate);
            mediump
            vec3 sum = currentColor.rgb;
            // 计算偏移坐标的颜色值总和
            for (int i = 0; i < SHIFT_SIZE; i++) {
                sum += texture2D(inputTexture, blurShiftCoordinates[i].xy).rgb;
                sum += texture2D(inputTexture, blurShiftCoordinates[i].zw).rgb;
            }
            // 求出平均值
            gl_FragColor = vec4(sum * 1.0 / float(2 * SHIFT_SIZE + 1), currentColor.a);
        }
);

GLGaussianPassBlurFilter::GLGaussianPassBlurFilter(
) : mBlurSize(1.0f),
    mVertexShader(nullptr),
    mFragmentShader(nullptr) {}

GLGaussianPassBlurFilter::GLGaussianPassBlurFilter(
        const char *vertexShader,
        const char *fragmentShader
) : mBlurSize(1.0f),
    mVertexShader(vertexShader),
    mFragmentShader(fragmentShader) {}

GLGaussianPassBlurFilter::~GLGaussianPassBlurFilter() {}

void GLGaussianPassBlurFilter::initProgram() {
    if (isInitialized()) {
        return;
    }
    if (mVertexShader != nullptr && mFragmentShader != nullptr) {
        initProgram(mVertexShader, mFragmentShader);
    } else {
        initProgram(kGaussianPassVertexShader.c_str(), kGaussianPassFragmentShader.c_str());
    }
}

void GLGaussianPassBlurFilter::initProgram(const char *vertexShader, const char *fragmentShader) {
    GLFilter::initProgram(vertexShader, fragmentShader);
    if (isInitialized()) {
        mTexelWidthOffsetHandle = glGetUniformLocation(mProgramHandle, "texelWidthOffset");
        mTexelHeightOffsetHandle = glGetUniformLocation(mProgramHandle, "texelHeightOffset");
    } else {
        mTexelWidthOffsetHandle = -1;
        mTexelHeightOffsetHandle = -1;
    }
}

void GLGaussianPassBlurFilter::setBlurSize(float blurSize) {
    this->mBlurSize = blurSize;
}

void GLGaussianPassBlurFilter::onDrawBegin() {
    GLFilter::onDrawBegin();
    if (isInitialized()) {
        if (mTextureWidth > 0) {
            glUniform1f(mTexelWidthOffsetHandle, mBlurSize / mTextureWidth);
        } else {
            glUniform1f(mTexelWidthOffsetHandle, 0.0f);
        }
        if (mTextureHeight > 0) {
            glUniform1f(mTexelHeightOffsetHandle, mBlurSize / mTextureHeight);
        } else {
            glUniform1f(mTexelHeightOffsetHandle, 0.0f);
        }
    }
}
