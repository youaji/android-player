
#include "GLIntensityFilter.h"

GLIntensityFilter::GLIntensityFilter() : intensityHandle(1.0f) {

}

void GLIntensityFilter::initProgram(const char *vertexShader, const char *fragmentShader) {
    GLFilter::initProgram(vertexShader, fragmentShader);
    if (isInitialized()) {
        intensityHandle = glGetUniformLocation(mProgramHandle, "intensity");
    }
}

void GLIntensityFilter::onDrawBegin() {
    GLFilter::onDrawBegin();
    if (isInitialized()) {
        glUniform1f(intensityHandle, mIntensity);
    }
}

