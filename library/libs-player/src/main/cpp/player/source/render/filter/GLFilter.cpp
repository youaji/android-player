
#include <cstdlib>
#include "GLFilter.h"
#include "render/common/header/OpenGLUtils.h"
#include <AndroidLog.h>

GLFilter::GLFilter() : mInitialized(false),
                       mProgramHandle(-1),
                       mPositionHandle(-1),
                       mTexCoordinateHandle(-1),
                       nbTextures(1),
                       mVertexCount(4),
                       mTimeStamp(0),
                       mIntensity(1.0),
                       mTextureWidth(0), mTextureHeight(0),
                       mDisplayWidth(0), mDisplayHeight(0) {

    for (int i = 0; i < MAX_TEXTURES; ++i) {
        // 初始化纹理句柄
        mInputTextureHandle[i] = -1;
    }
}

GLFilter::~GLFilter() {}

void GLFilter::initProgram() {
    if (!isInitialized()) {
        initProgram(kDefaultVertexShader.c_str(), kDefaultFragmentShader.c_str());
    }
}

void GLFilter::initProgram(const char *vertexShader, const char *fragmentShader) {
    if (isInitialized()) {
        return;
    }
    if (vertexShader && fragmentShader) {
        mProgramHandle = OpenGLUtils::createProgram(vertexShader, fragmentShader);
        LOGD("GLFilter->着色器程序id[%d]", mProgramHandle);
        // OpenGLUtils::checkGLError("createProgram");
        // 获取着色器程序中，指定为attribute类型变量的id
        mPositionHandle = glGetAttribLocation(mProgramHandle, "aPosition");
        mTexCoordinateHandle = glGetAttribLocation(mProgramHandle, "aTextureCoord");
        // 纹理句柄
        mInputTextureHandle[0] = glGetUniformLocation(mProgramHandle, "inputTexture");
        setInitialized(true);
    } else {
        mPositionHandle = -1;
        mPositionHandle = -1;
        mInputTextureHandle[0] = -1;
        setInitialized(false);
    }
}

void GLFilter::destroyProgram() {
    if (mInitialized) {
        glDeleteProgram(mProgramHandle);
    }
    mProgramHandle = -1;
}

void GLFilter::setInitialized(bool initialized) {
    this->mInitialized = initialized;
}

bool GLFilter::isInitialized() {
    return mInitialized;
}

void GLFilter::setTextureSize(int width, int height) {
    this->mTextureWidth = width;
    this->mTextureHeight = height;
}

void GLFilter::setDisplaySize(int width, int height) {
    this->mDisplayWidth = width;
    this->mDisplayHeight = height;
}

void GLFilter::setTimeStamp(double timeStamp) {
    this->mTimeStamp = timeStamp;
}

void GLFilter::setIntensity(float intensity) {
    this->mIntensity = intensity;
}

void GLFilter::updateViewPort() {
    if (mDisplayWidth != 0 && mDisplayHeight != 0) {
        //LOGD("GLFilter->视口宽高%d，%d",displayWidth,displayHeight);
        glViewport(0, 0, mDisplayWidth, mDisplayHeight);
    } else {
        glViewport(0, 0, mTextureWidth, mTextureHeight);
    }
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void
GLFilter::drawTexture(FrameBuffer *frameBuffer, GLuint texture, const float *vertices, const float *textureVertices) {
    // 绑定FBO以后，就会把纹理渲染结果保存到FBO缓冲区而不是系统的缓冲区
    if (frameBuffer) {
        frameBuffer->bindBuffer();
    }
    drawTexture(texture, vertices, textureVertices, false);
    // unbindBuffer取消绑定，相当于返回默认的帧缓冲区
    if (frameBuffer) {
        frameBuffer->unbindBuffer();
    }
}

void GLFilter::bindAttributes(const float *vertices, const float *textureVertices) {
    // 告诉GPU怎么解析顶点数据，最后一个参数是顶点数组，OpenGL内部会根据这个顶点数组和指定的规则计算出视口坐标系中每个位置的顶点坐标值
    // 然后根据对应的纹理坐标计算出每个顶点坐标对应的纹理坐标是什么，这就可以从纹理坐标系中取出对应的纹素值，将其渲染到对应的顶点坐标中
    glVertexAttribPointer(mPositionHandle, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    // 告诉GPU怎么解析纹理坐标数据
    glVertexAttribPointer(mTexCoordinateHandle, 2, GL_FLOAT, GL_FALSE, 0, textureVertices);

    glEnableVertexAttribArray(mPositionHandle);
    glEnableVertexAttribArray(mTexCoordinateHandle);
}

void GLFilter::onDrawBegin() {
}

void GLFilter::onDrawAfter() {
    // do nothing
}

void GLFilter::onDrawFrame() {
    // GL_TRIANGLE_STRIP表示顺序在每三个顶点之间均绘制三角形，这个方法可以保证从相同的方向上所有三角形均被绘制
    // 以V0V1V2,V1V2V3,V2V3V4……的形式绘制三角形
    glDrawArrays(GL_TRIANGLE_STRIP, 0, mVertexCount);
}

void GLFilter::unbindAttributes() {
    glDisableVertexAttribArray(mTexCoordinateHandle);
    glDisableVertexAttribArray(mPositionHandle);
}

void GLFilter::bindTexture(GLuint texture) {
    // 激活0号纹理单元
    glActiveTexture(GL_TEXTURE0);
    // 将纹理对象跟激活的纹理单元绑定
    glBindTexture(getTextureType(), texture);
    // 将纹理单元跟着色器句柄关联，这样着色器就可以通过纹理单元来操作纹理对象texture了
    glUniform1i(mInputTextureHandle[0], 0);
}

void GLFilter::unbindTextures() {
    // 默认解绑纹理单元0
    glBindTexture(getTextureType(), 0);
}

void GLFilter::drawTexture(GLuint texture, const float *vertices, const float *textureVertices,
                           bool viewPortUpdate) {
    if (!isInitialized() || texture < 0) {
        return;
    }

    // 设置视口大小
    if (viewPortUpdate) {
        updateViewPort();
    }

    // 绑定program
    glUseProgram(mProgramHandle);
    // 绑定纹理
    bindTexture(texture);
    // 绑定属性值
    bindAttributes(vertices, textureVertices);
    // 绘制前处理
    onDrawBegin();
    // 绘制纹理
    onDrawFrame();
    // 绘制后处理
    onDrawAfter();
    // 解绑属性
    unbindAttributes();
    // 解绑纹理
    unbindTextures();
    // 解绑program
    glUseProgram(0);

}

GLenum GLFilter::getTextureType() {
    return GL_TEXTURE_2D;
}
