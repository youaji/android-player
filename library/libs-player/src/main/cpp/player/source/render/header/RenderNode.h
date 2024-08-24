#ifndef FFMPEG4_RENDERNODE_H
#define FFMPEG4_RENDERNODE_H

#include <string>

#include "FrameBuffer.h"

#include "NodeType.h"
#include "GLOutFilter.h"

/**
 * 渲染结点
 * 数据流如下：
 *   input->
 *      render node 1->
 *      render node 2->
 *      render node 3->
 *      ...->
 *      render node n->
 *      output texture->
 *      frameBuffer 1->
 *      frameBuffer 2->
 *      frameBuffer 3->
 *      ...->
 *      frameBuffer n->
 *      texture ->
 *      glFilter ->glFilter ->glFilter -> ...->glFilter ->glFilter
 *
 * 每个渲染结点上的glFilter均可以随时改变，而FrameBuffer则不需要跟随glFilter一起销毁重建，节省开销
 */
class RenderNode {

public:
    RenderNode(RenderNodeType type);

    virtual ~RenderNode();

    /**
     * 初始化
     */
    void init();

    /**
     * 销毁结点
     */
    void destroy();

    /**
     * 设置纹理大小
     * @param width
     * @param height
     */
    void setTextureSize(int width, int height);

    /**
     * 设置显示界面的宽高大小
     * @param width
     * @param height
     */
    void setDisplaySize(int width, int height);

    /**
     * 设置FrameBuffer
     * @param buffer
     */
    void setFrameBuffer(FrameBuffer *buffer);

    /**
     * 切换Filter
     * @param filter
     */
    void changeFilter(GLFilter *filter);

    /**
     * 设置时间戳
     * @param timeStamp
     */
    void setTimeStamp(double timeStamp);

    /**
     * 设置强度
     * @param intensity
     */
    void setIntensity(float intensity);

    /**
     * 直接绘制输出
     * @param texture
     * @param vertices
     * @param textureVertices
     * @return
     */
    virtual bool drawFrame(GLuint texture, const float *vertices, const float *textureVertices);

    /**
     * 绘制到FBO
     * @param texture
     * @param vertices
     * @param textureVertices
     * @return
     */
    virtual int drawFrameBuffer(GLuint texture, const float *vertices, const float *textureVertices);

    /**
     * @return
     */
    RenderNodeType getNodeType() const;

    /**
     * @return
     */
    bool hasFrameBuffer() const;


public:
    // 前继结点
    RenderNode *prevNode;
    // 下一个渲染结点
    RenderNode *nextNode;
    // 渲染结点的类型
    RenderNodeType nodeType;
    // 滤镜
    GLFilter *glFilter;

protected:
    int textureWidth, textureHeight;// 纹理宽高
    int displayWidth, displayHeight;// 显示宽高
    FrameBuffer *frameBuffer;       // FrameBuffer 对象
};

#endif //FFMPEG4_RENDERNODE_H
