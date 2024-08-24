#ifndef FFMPEG4_OPENGLUTILS_H
#define FFMPEG4_OPENGLUTILS_H

#if defined(__ANDROID__)

#include <stdio.h>
#include <stdlib.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2platform.h>

#endif

#include "glm.hpp"
#include <gtc/matrix_transform.hpp>
#include <string>

class OpenGLUtils {
public:
    /**
     * 创建program
     * @param vertexShader
     * @param fragShader
     * @return
     */
    static GLuint createProgram(const char *vertexShader, const char *fragShader);

    /**
     * 加载shader
     * @param type
     * @param shaderSrc
     * @return
     */
    static GLuint loadShader(GLenum type, const char *shaderSrc);

    /**
     * 查询活动的统一变量uniform
     * @param program
     */
    static void checkActiveUniform(GLuint program);

    /**
     * 创建texture
     * @param type
     * @return
     */
    static GLuint createTexture(GLenum type);

    /**
     * 创建texture
     * @param bytes
     * @param width
     * @param height
     * @return
     */
    static GLuint createTextureWithBytes(unsigned char *bytes, int width, int height);

    /**
     * 使用旧的Texture 创建新的Texture
     * @param texture
     * @param bytes
     * @param width
     * @param height
     * @return
     */
    static GLuint createTextureWithOldTexture(GLuint texture, unsigned char *bytes, int width, int height);

    /**
     * 创建一个FBO和Texture
     * @param frameBuffer
     * @param texture
     * @param width
     * @param height
     */
    static void createFrameBuffer(GLuint *frameBuffer, GLuint *texture, int width, int height);

    /**
     * 创建FBO和Texture
     * @param frameBuffers
     * @param textures
     * @param width
     * @param height
     * @param size
     */
    static void createFrameBuffers(GLuint *frameBuffers, GLuint *textures, int width, int height, int size);

    /**
     * 检查是否出错
     * @param op
     */
    static void checkGLError(const char *op);

    /**
     * 绑定纹理
     * @param location
     * @param texture
     * @param index
     */
    static void bindTexture(int location, int texture, int index);

    /**
     * 绑定纹理，指定纹理类型
     * @param location
     * @param texture
     * @param index
     * @param textureType
     */
    static void bindTexture(int location, int texture, int index, int textureType);

    /**
     * 计算屏幕适配矩阵
     * @param videoWidth
     * @param videoHeight
     * @param displayWidth
     * @param displayHeight
     * @return
     */
    static glm::mat4 calculateVideoFitMat4(int videoWidth, int videoHeight, int displayWidth, int displayHeight);

//    /**
//     * 从资源文件中读取shader代码
//     * @param fileName
//     * @return
//     */
//    static std::string *readShaderFromAsset(const char *fileName);

private:
    OpenGLUtils() = default;

    virtual ~OpenGLUtils() {}
};

#endif //FFMPEG4_OPENGLUTILS_H
