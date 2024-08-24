
#ifndef FFMPEG4_GLINPUTYUV420PFILTER_H
#define FFMPEG4_GLINPUTYUV420PFILTER_H

#include "GLInputFilter.h"

/**
 * YUV420P输入滤镜
 */
class GLInputYUV420PFilter : public GLInputFilter {
public:
    GLInputYUV420PFilter();

    virtual ~GLInputYUV420PFilter();

    void initProgram() override;

    void initProgram(const char *vertexShader, const char *fragmentShader) override;

    GLboolean renderTexture(Texture *texture, float *vertices, float *textureVertices) override;

    GLboolean uploadTexture(Texture *texture) override;
};

#endif //FFMPEG4_GLINPUTYUV420PFILTER_H
