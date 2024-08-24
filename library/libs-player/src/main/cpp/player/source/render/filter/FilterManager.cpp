#include <cstring>
#include <render/filter/effect/header/GLEffectScaleFilter.h>
#include <render/filter/effect/header/GLFrameThreeFilter.h>
#include <render/filter/effect/header/GLFrameTwoFilter.h>
#include <render/filter/effect/header/GLFrameBlurFilter.h>
#include <render/filter/effect/header/GLFrameBlackWhiteThreeFilter.h>
#include <render/filter/effect/header/GLFrameFourFilter.h>
#include <render/filter/effect/header/GLFrameSixFilter.h>
#include <render/filter/effect/header/GLFrameNineFilter.h>
#include <render/filter/effect/header/GLEffectPipFilter.h>
#include <render/filter/effect/header/GLEffectRotateCircleFilter.h>
#include "GLBackGroundFilter.h"
#include "GLWatermarkFilter.h"
#include "FilterManager.h"

FilterManager *FilterManager::instance = 0;
std::mutex FilterManager::mutex;

FilterManager::FilterManager() {

}

FilterManager::~FilterManager() {

}

FilterManager *FilterManager::getInstance() {
    if (!instance) {
        std::unique_lock<std::mutex> lock(mutex);
        if (!instance) {
            instance = new(std::nothrow) FilterManager();
        }
    }
    return instance;
}

void FilterManager::destroy() {
    if (instance) {
        std::unique_lock<std::mutex> lock(mutex);
        if (instance) {
            delete instance;
            instance = nullptr;
        }
    }
}

GLFilter *FilterManager::getFilter(FilterInfo *filterInfo) {

    if (filterInfo->id != -1) {
        return getFilter(filterInfo->id);
    } else if (filterInfo->name != nullptr) {
        return getFilter(filterInfo->name);
    }
    return nullptr;
}

GLFilter *FilterManager::getFilter(const char *name) {
    // 背景颜色
    if (!strcmp("原色", name)) {
        GLint type = 0;
        GLfloat colors[3] = {0.0f, 0.0f, 0.0f};
        return new GLBackGroundFilter(type, colors);
    }
    if (!strcmp("黑白", name)) {
        GLint type = 1;
        GLfloat colors[3] = {0.299f, 0.587f, 0.114f};
        return new GLBackGroundFilter(type, colors);
    }
    if (!strcmp("暖色调", name)) {
        GLint type = 2;
        GLfloat colors[3] = {0.1f, 0.1f, 0.0f};
        return new GLBackGroundFilter(type, colors);
    }
    if (!strcmp("冷色调", name)) {
        GLint type = 2;
        GLfloat colors[3] = {0.0f, 0.0f, 0.1f};
        return new GLBackGroundFilter(type, colors);
    }
    if (!strcmp("模糊", name)) {
        GLint type = 3;
        GLfloat colors[3] = {0.006f, 0.004f, 0.002f};
        return new GLBackGroundFilter(type, colors);
    }

    // 滤镜特效
    if (!strcmp("灵魂出窍", name)) {
        return new GLEffectSoulStuffFilter();
    }
    if (!strcmp("抖动", name)) {
        return new GLEffectShakeFilter();
    }
    if (!strcmp("幻觉", name)) {
        return new GLEffectIllusionFilter();
    }
    if (!strcmp("缩放", name)) {
        return new GLEffectScaleFilter();
    }
    if (!strcmp("闪白", name)) {
        return new GLEffectGlitterWhiteFilter();
    }

    // 分屏特效
    if (!strcmp("模糊分屏", name)) {
        return new GLFrameBlurFilter();
    }
    if (!strcmp("黑白三屏", name)) {
        return new GLFrameBlackWhiteThreeFilter();
    }
    if (!strcmp("两屏", name)) {
        return new GLFrameTwoFilter();
    }
    if (!strcmp("三屏", name)) {
        return new GLFrameThreeFilter();
    }
    if (!strcmp("四屏", name)) {
        return new GLFrameFourFilter();
    }
    if (!strcmp("六屏", name)) {
        return new GLFrameSixFilter();
    }
    if (!strcmp("九屏", name)) {
        return new GLFrameNineFilter();
    }
    if (!strcmp("画中画", name)) {
        return new GLEffectPipFilter();
    }
    if (!strcmp("画中圆", name)) {
        return new GLEffectRotateCircleFilter();
    }

    return nullptr;
}

GLFilter *FilterManager::getFilter(const int id) {
    switch (id) {
        /*
         * 滤镜特效
         */
        case 0x000: { // 灵魂出窍
            return getFilter("灵魂出窍");
//            return new GLEffectSoulStuffFilter();
        }
        case 0x001: { // 抖动
            return getFilter("抖动");
//            return new GLEffectShakeFilter();
        }
        case 0x002: { // 幻觉
            return getFilter("幻觉");
//            return new GLEffectIllusionFilter();
        }
        case 0x003: { // 缩放
            return getFilter("缩放");
//            return new GLEffectScaleFilter();
        }
        case 0x004: { // 闪白
            return getFilter("闪白");
//            return new GLEffectGlitterWhiteFilter();
        }

            /*
             * 背景颜色
             */
        case 0x100: { // 原色
            return getFilter("原色");
        }
        case 0x101: { // 黑白
            return getFilter("黑白");
        }
        case 0x102: { // 暖色调
            return getFilter("暖色调");
        }
        case 0x103: { // 冷色调
            return getFilter("冷色调");
        }
        case 0x104: { // 模糊
            return getFilter("模糊");
        }

            /*
             * 分屏特效
             */
        case 0x200: { // 模糊分屏特效
            return getFilter("模糊分屏");
//            return new GLFrameBlurFilter();
        }
        case 0x201: { // 黑白三屏特效
            return getFilter("黑白三屏");
//            return new GLFrameBlackWhiteThreeFilter();
        }
        case 0x202: { // 两屏特效
            return getFilter("两屏");
//            return new GLFrameTwoFilter();
        }
        case 0x203: { // 三屏特效
            return getFilter("三屏");
//            return new GLFrameThreeFilter();
        }
        case 0x204: { // 四屏特效
            return getFilter("四屏");
//            return new GLFrameFourFilter();
        }
        case 0x205: { // 六屏特效
            return getFilter("六屏");
//            return new GLFrameSixFilter();
        }
        case 0x206: { // 九屏特效
            return getFilter("九屏");
//            return new GLFrameNineFilter();
        }
        case 0x207: { // 画中画特效
            return getFilter("画中画");
//            return new GLEffectPipFilter();
        }
        case 0x208: { // 画中圆特效
            return getFilter("画中圆");
//            return new GLEffectRotateCircleFilter();
        }
    }
    return nullptr;
}
