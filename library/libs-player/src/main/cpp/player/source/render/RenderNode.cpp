#include "RenderNode.h"

RenderNode::RenderNode(RenderNodeType type) : prevNode(nullptr), nextNode(nullptr), glFilter(nullptr),
                                              frameBuffer(nullptr), nodeType(type), textureWidth(-1), textureHeight(-1),
                                              displayWidth(-1), displayHeight(-1) {}

RenderNode::~RenderNode() {}

void RenderNode::init() {
    // 如果还没有初始化，则初始化
    if (glFilter != nullptr && !glFilter->isInitialized()) {
        glFilter->initProgram(); //初始化程序
    }
}


void RenderNode::destroy() {
    // 释放filter的shader program
    if (glFilter != nullptr) {
        glFilter->destroyProgram();
    }
    // 将FrameBuffer放入管理器中
    if (frameBuffer != nullptr) {
        frameBuffer->destroy();
        delete frameBuffer;
        frameBuffer = nullptr;
    }
}

void RenderNode::setTextureSize(int width, int height) {
    if (textureWidth != width || textureHeight != height) {
        textureWidth = width;
        textureHeight = height;
    }
    if (glFilter) {
        glFilter->setTextureSize(width, height);
    }
}

void RenderNode::setDisplaySize(int width, int height) {
    displayWidth = width;
    displayHeight = height;
    if (displayWidth != 0 && displayHeight != 0) {
        glFilter->setDisplaySize(displayWidth, displayHeight);
    }
}

void RenderNode::setFrameBuffer(FrameBuffer *buffer) {
    // 删除旧FBO
    if (this->frameBuffer != nullptr) {
        frameBuffer->destroy();
        delete frameBuffer;
    }
    this->frameBuffer = buffer;
}

void RenderNode::changeFilter(GLFilter *filter) {
    if (this->glFilter != nullptr) {
        this->glFilter->destroyProgram();
        delete this->glFilter;
    }

    this->glFilter = filter;

    // 如果还没有初始化，则初始化
//    if (glFilter && !glFilter->isInitialized()) {
//        glFilter->initProgram();
//    }
}

void RenderNode::setTimeStamp(double timeStamp) {
    if (glFilter != nullptr) {
        glFilter->setTimeStamp(timeStamp);
    }
}

void RenderNode::setIntensity(float intensity) {
    if (glFilter != nullptr) {
        glFilter->setIntensity(intensity);
    }
}

bool RenderNode::drawFrame(GLuint texture, const float *vertices, const float *textureVertices) {
    if (!glFilter || !glFilter->isInitialized()) {
        return false;
    }
    glFilter->drawTexture(texture, vertices, textureVertices);
    return true;
}

int RenderNode::drawFrameBuffer(GLuint texture, const float *vertices, const float *textureVertices) {
    // FrameBuffer没有或者是滤镜还没初始化，则直接返回输入的纹理
    if (!frameBuffer || !frameBuffer->isInitialized() || !glFilter || !glFilter->isInitialized()) {
        return texture;
    }

    glFilter->drawTexture(frameBuffer, texture, vertices, textureVertices);
    return frameBuffer->getTexture();
}

RenderNodeType RenderNode::getNodeType() const {
    return nodeType;
}

bool RenderNode::hasFrameBuffer() const {
    return (frameBuffer != nullptr);
}

