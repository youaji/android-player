#include <AndroidLog.h>
#include "CoordinateUtils.h"
#include "DisplayRenderNode.h"
#include "GLESDevice.h"
#include "GLWatermarkFilter.h"


GLESDevice::GLESDevice() {
    mNativeWindow = NULL;
    mSurfaceWidth = 0;
    mSurfaceHeight = 0;
    mEGLSurface = EGL_NO_SURFACE;
    mEglHelper = new EglHelper();

    mIsHasEGLSurface = false;
    mIsHasEGlContext = false;
    mIsHasSurface = false;
    // 分配纹理数据的内存
    mVideoTexture = (Texture *) malloc(sizeof(Texture));
    memset(mVideoTexture, 0, sizeof(Texture));
    mInputRenderNode = NULL;

    mNodeList = new RenderNodeList();
    mNodeList->addNode(new DisplayRenderNode());     // 添加显示渲染的结点
    memset(&mFilterInfo, 0, sizeof(FilterInfo));
    mFilterInfo.type = NODE_NONE;
    mFilterInfo.name = nullptr;
    mFilterInfo.id = -1;
    mIsFilterChange = true;

    resetVertices();
    resetTextureVertices();
}

GLESDevice::~GLESDevice() {
    mMutex.lock();
    terminate();
    free(mVideoTexture);
    delete mNodeList;
    mMutex.unlock();
}

void GLESDevice::surfaceCreated(ANativeWindow *window) {
    mMutex.lock();
    if (mNativeWindow != NULL) {
        ANativeWindow_release(mNativeWindow);
        mNativeWindow = NULL;
        // 重置window则需要重置Surface
        mIsSurfaceReset = true;
    }
    mNativeWindow = window;
    if (mNativeWindow != NULL) {
        // 渲染区域的宽高
        mSurfaceWidth = ANativeWindow_getWidth(mNativeWindow);
        mSurfaceHeight = ANativeWindow_getHeight(mNativeWindow);
    }
    mIsHasSurface = true;
    mMutex.unlock();
}

void GLESDevice::surfaceChanged(int width, int height) {
    mMutex.lock();
    mSurfaceWidth = width;
    mSurfaceHeight = height;
    if (mNodeList != nullptr && mNodeList->findNode(NODE_DISPLAY) != nullptr) {
        // 对象在指针强转要注意，比如这里的findNode返回的RenderNode *也是可以强转为GLOutFilter *的，但是不能使用，编译切没有问题
        GLOutFilter *glOutFilter = (GLOutFilter *) mNodeList->findNode(NODE_DISPLAY)->glFilter;
        glOutFilter->nativeSurfaceChanged(width, height);
    }
    mMutex.unlock();
}


void GLESDevice::changeFilter(RenderNodeType type, const char *filterName) {
    mMutex.lock();
    mFilterInfo.type = type;
    mFilterInfo.name = av_strdup(filterName);
    mFilterInfo.id = -1;
    mIsFilterChange = true;
    mMutex.unlock();
}

void GLESDevice::changeFilter(RenderNodeType type, const int id) {
    mMutex.lock();
    mFilterInfo.type = type;
    if (mFilterInfo.name) {
        av_freep(&mFilterInfo.name);
        mFilterInfo.name = nullptr;
    }
    mFilterInfo.name = nullptr;
    mFilterInfo.id = id;
    mIsFilterChange = true;
    mMutex.unlock();
}

void GLESDevice::setWatermark(uint8_t *watermarkPixel, size_t length, GLint width, GLint height, GLfloat scale, GLint location) {
    mMutex.lock();
    RenderNode *node = mNodeList->findNode(NODE_STICKERS);
    if (!node) {
        node = new RenderNode(NODE_STICKERS);
    }
    // 创建水印滤镜
    GLWatermarkFilter *watermarkFilter = new GLWatermarkFilter();
    watermarkFilter->setTextureSize(mVideoTexture->width, mVideoTexture->height);
    watermarkFilter->setDisplaySize(mSurfaceWidth, mSurfaceHeight);
    watermarkFilter->setWatermark(watermarkPixel, length, width, height, scale, location);
    node->changeFilter(watermarkFilter);
    mNodeList->addNode(node); // 添加水印节点
    // 要在 onInitTexture 中初始化水印 filter，
    // 可能原因是egl属性的创建(比如着色器program，elgSurface 等)和使用需要在同一个线程中完成
    mFilterInfo.type = NODE_NONE;
    mFilterInfo.name = nullptr;
    mFilterInfo.id = -1;
    mIsFilterChange = true;
    mMutex.unlock();
}

void GLESDevice::terminate(bool releaseContext) {
    // 销毁 Surface
    if (mEGLSurface != EGL_NO_SURFACE) {
        mEglHelper->destroySurface(mEGLSurface);
        mEGLSurface = EGL_NO_SURFACE;
        mIsHasEGLSurface = false;
    }

    // 销毁egl上下文
    if (mEglHelper->getEglContext() != EGL_NO_CONTEXT && releaseContext) {
        if (mInputRenderNode != NULL) { //释放输入节点
            mInputRenderNode->destroy();
            delete mInputRenderNode;
        }
        mEglHelper->release();
        delete mEglHelper;
        if (mNativeWindow != NULL) {
            ANativeWindow_release(mNativeWindow);
            mNativeWindow = NULL;
        }
        mIsHasEGlContext = false;
    }
}

void GLESDevice::terminate() {
    terminate(true);
}

void GLESDevice::setTimeStamp(double timeStamp) {
    mMutex.lock();
    if (mNodeList) {
        mNodeList->setTimeStamp(timeStamp);
    }
    mMutex.unlock();
}

void GLESDevice::onInitTexture(int width, int height, TextureFormat format, BlendMode blendMode, int rotate) {
    mMutex.lock();
    // 创建EGLContext
    if (!mIsHasEGlContext) {
        // 在这个init方法中会调用到egl中的相关初始化操作，
        // 包括创建和初始化EGLDisplay、初始化图形上下文EGLContext
        mIsHasEGlContext = mEglHelper->init(FLAG_TRY_GLES3);
        LOGD("GLESDevice->isHasEGlContext : %d", mIsHasEGlContext);
    }

    if (!mIsHasEGlContext) {
        return;
    }
    // 是否需要重置Surface，兼容SurfaceHolder处理
    if (mIsHasSurface && mIsSurfaceReset) {
        terminate(false);
        mIsSurfaceReset = false;
    }

    // 创建EGLSurface
    if (mEGLSurface == EGL_NO_SURFACE && mNativeWindow != NULL) {
        // 如果已经有Surface但是还没有mEGLSurface，则创建之
        if (mIsHasSurface && !mIsHasEGLSurface) {
            mEGLSurface = mEglHelper->createSurface(mNativeWindow);
            if (mEGLSurface != EGL_NO_SURFACE) {
                mIsHasEGLSurface = true;
                LOGD("GLESDevice->isHasEGLSurface : %d", mIsHasEGLSurface);
            }
        }
    } else if (mEGLSurface != EGL_NO_SURFACE && mIsHasEGLSurface) {
        // 处于SurfaceDestroyed状态，释放EGLSurface
        if (!mIsHasSurface) {
            terminate(false);
        }
    }

    // 计算帧的宽高，如果不相等，则需要重新计算缓冲区的大小
    if (mNativeWindow != NULL && mSurfaceWidth != 0 && mSurfaceHeight != 0) {

        // 当视频宽高比和窗口宽高比例不一致时，调整缓冲区的大小
        if ((mSurfaceWidth / mSurfaceHeight) != (width / height)) {
            // 让视口的宽高比适应视频的宽高比
            // mSurfaceHeight = mSurfaceWidth * height / width; // 这个不注释会有问题
            int windowFormat = ANativeWindow_getFormat(mNativeWindow);
            // 设置缓冲区参数，把width和height设置成你要显示的图片的宽和高
            ANativeWindow_setBuffersGeometry(mNativeWindow, 0, 0, windowFormat);
            // ANativeWindow_setBuffersGeometry(mNativeWindow, mSurfaceWidth, mSurfaceHeight, windowFormat);
        }
    }

    mVideoTexture->rotate = rotate;
    // LOGE("GLESDevice->帧的宽度%d",width)
    // 视频帧的宽高
    mVideoTexture->frameWidth = width;
    mVideoTexture->frameHeight = height;
    // 纹理高度
    mVideoTexture->height = height;
    mVideoTexture->width = width;
    mVideoTexture->format = format;
    mVideoTexture->blendMode = blendMode;
    mVideoTexture->direction = FLIP_NONE;
    // 关联egl上下文
    mEglHelper->makeCurrent(mEGLSurface);

    // 第一次会初始化，这个初始化主要作用是，创建对应的着色器程序和对应的纹理对象
    if (mInputRenderNode == NULL) {
        // 初始化输入节点
        mInputRenderNode = new InputRenderNode();
        if (mInputRenderNode != NULL) {
            // 输入节点的初始化
            mInputRenderNode->initFilter(mVideoTexture);

            // 创建一个FBO给渲染节点
            FrameBuffer *frameBuffer = new FrameBuffer(width, height);
            // 初始化主要是创建一个帧缓冲区FBO并挂载一个颜色纹理
            frameBuffer->init();
            mInputRenderNode->setFrameBuffer(frameBuffer);

            // 设置所有节点的视口大小
            if (mSurfaceWidth != 0 && mSurfaceHeight != 0) {
                mNodeList->setDisplaySize(mSurfaceWidth, mSurfaceHeight);
            }
        }
    }
    // 如果改变了滤镜效果的渲染，则在节点链表中增加或更改为当前的滤镜
    if (mIsFilterChange) {
        // 改变滤镜
        mNodeList->changeFilter(mFilterInfo.type, FilterManager::getInstance()->getFilter(&mFilterInfo));
        mIsFilterChange = false;
        // 节点的视口大小
        if (mSurfaceWidth != 0 && mSurfaceHeight != 0) {
            mNodeList->setDisplaySize(mSurfaceWidth, mSurfaceHeight);
        }
        // 设置纹理大小，根据纹理大小初始化FBO
        mNodeList->setTextureSize(width, height);
        // 节点初始化，主要是filter的初始化
        mNodeList->init(); //filter初始化了以后才能用
    }
    mMutex.unlock();
}

int GLESDevice::onUpdateYUV(uint8_t *yData, int yPitch, uint8_t *uData, int uPitch, uint8_t *vData, int vPitch) {
    if (!mIsHasEGlContext) {
        return -1;
    }
    mMutex.lock();
    // 分别赋值YUV的宽度
    mVideoTexture->pitches[0] = yPitch;
    mVideoTexture->pitches[1] = uPitch;
    mVideoTexture->pitches[2] = vPitch;
    // 分别赋值YUV的像素数据
    mVideoTexture->pixels[0] = yData;
    mVideoTexture->pixels[1] = uData;
    mVideoTexture->pixels[2] = vData;

    if (mInputRenderNode != NULL && mEGLSurface != EGL_NO_SURFACE) {
        mEglHelper->makeCurrent(mEGLSurface);
        mInputRenderNode->uploadTexture(mVideoTexture);
    }
    // LOGE("GLESDevice->纹理的宽度%d",yPitch)
    // 设置像素实际的宽度，即 lineSize 的值
    mVideoTexture->width = yPitch;
    mMutex.unlock();
    return 0;
}

int GLESDevice::onUpdateARGB(uint8_t *rgba, int pitch) {
    if (!mIsHasEGlContext) {
        return -1;
    }
    mMutex.lock();
    // ARGB模式
    mVideoTexture->pitches[0] = pitch; // lineSize
    mVideoTexture->pixels[0] = rgba; // data

    if (mInputRenderNode != NULL && mEGLSurface != EGL_NO_SURFACE) {
        mEglHelper->makeCurrent(mEGLSurface);
        mInputRenderNode->uploadTexture(mVideoTexture);
    }
    // LOGE("GLESDevice->纹理的宽度%d",pitch/4);
    // 设置像素实际的宽度，即 lineSize 的值，
    // 因为 BGRA_8888 中一个像素为 4 个字节，所以这里像素长度等于字节长度除以 4
    mVideoTexture->width = pitch / 4;
    mMutex.unlock();
    return 0;
}

int GLESDevice::onRequestRender(bool flip) {
    if (!mIsHasEGlContext) {
        return -1;
    }
    mMutex.lock();
    mVideoTexture->direction = flip ? FLIP_VERTICAL : FLIP_NONE;
    // LOGD("GLESDevice->flip ? %d", flip);
    if (mInputRenderNode != NULL && mEGLSurface != EGL_NO_SURFACE) {
        mEglHelper->makeCurrent(mEGLSurface);

        // 第一步是输入节点的渲染，mRenderNode就是输入节点，输入节点也有一个FBO，此时将渲染结果保存到FBO中
        int texture = mInputRenderNode->drawFrameBuffer(mVideoTexture);

//        if (mSurfaceWidth != 0 && mSurfaceHeight != 0) {
//            // 设置显示窗口的大小
//            mNodeList->setDisplaySize(mSurfaceWidth, mSurfaceHeight);
//            // LOGE("GLESDevice->window的宽高=%d，%d",ANativeWindow_getWidth(mNativeWindow),ANativeWindow_getHeight(mNativeWindow));
//        }
        // 渲染纹理，直接渲染到系统默认的帧缓冲区，而不是自定义的帧缓冲区FBO
        // mInputRenderNode->drawFrame(mVideoTexture);
        // 从渲染节点链表中的节点依次对纹理数据进行处理，并最后显示
        mNodeList->drawFrame(texture, mVertices, mTextureVertices);
        mEglHelper->swapBuffers(mEGLSurface);
    }
    mMutex.unlock();
    return 0;
}

void GLESDevice::resetVertices() {
    const float *verticesVertexCoordinates = CoordinateUtils::getVertexCoordinates();
    for (int i = 0; i < 8; ++i) {
        mVertices[i] = verticesVertexCoordinates[i];
    }
}

void GLESDevice::resetTextureVertices() {
    // 这个纹理坐标是FBO的纹理坐标，以左下角为原点
    const float *verticesTextureCoordinates = CoordinateUtils::getTextureCoordinates(ROTATE_NONE);
    for (int i = 0; i < 8; ++i) {
        mTextureVertices[i] = verticesTextureCoordinates[i];
    }
}
