
#ifndef FFMPEG4_GLOUTFILTER_H
#define FFMPEG4_GLOUTFILTER_H

#include "GLFilter.h"

// 输出节点的顶点着色器
const std::string kOutFilterVertexShader = SHADER_TO_STRING(
        precision mediump float; //默认精度
        //顶点坐标，x、y、z、w四个值
        attribute highp vec4 aPosition;
        //纹理坐标
        attribute highp vec2 aTextureCoord;
        //纹理坐标传递到片元着色器的变量，通过varying通道传递
        varying vec2 textureCoordinate;
        // 顶点坐标矩阵
        uniform mat4 vMatrix;

        void main() {
            // 通过矩阵改变顶点坐标，顶点坐标基于OpenGL坐标系，用来指定物体的摆放位置
            gl_Position = vMatrix * vec4(aPosition.x, aPosition.y, aPosition.z, aPosition.w);
            textureCoordinate = aTextureCoord.xy;
        }
);

class GLOutFilter : public GLFilter {

public:
    GLOutFilter();

    virtual ~GLOutFilter();

    void initProgram() override;

    void initProgram(const char *vertexShader, const char *fragmentShader) override;

    void onDrawBegin() override;

    //Surface的大小发生改变
    void nativeSurfaceChanged(int width, int height);

protected:
    int matrixHandle;                        // 顶点坐标的矩阵句柄
    glm::mat4 v_mat4 = glm::mat4(1.0f);   // 顶点矩阵
};

#endif //FFMPEG4_GLOUTFILTER_H