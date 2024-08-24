
#ifndef GLBEAUTYFILTER_H
#define GLBEAUTYFILTER_H


#include <GLGroupFilter.h>

class GLBeautyFilter : public GLGroupFilter {
public:
    GLBeautyFilter();

    virtual ~GLBeautyFilter();

    void drawTexture(GLuint texture, const float *vertices, const float *textureVertices,
                     bool viewPortUpdate = false) override;

    void drawTexture(FrameBuffer *frameBuffer, GLuint texture, const float *vertices,
                     const float *textureVertices) override;
};


#endif //GLBEAUTYFILTER_H
