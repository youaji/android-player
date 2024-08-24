package com.youaji.libs.player

/**
 * 背景颜色
 */
enum class Background(val id: Int, val desc: String) {
    NONE(0x100, "原色"),
    GRAY(0x101, "黑白"),
    COOL(0x102, "冷色调"),
    WARM(0x103, "暖色调"),
    BLUR(0x104, "模糊");
}

/**
 * 滤镜类型
 */
enum class Filter(val id: Int, val desc: String) {
    SOUL(0x000, "灵魂出窍"),
    SHAKE(0x001, "抖动"),
    ILLUSION(0x002, "幻觉"),
    SCALE(0x003, "缩放"),
    GLITTER_WHITE(0x004, "闪白"),
}

/**
 * 特效类型
 */
enum class Effect(val id: Int, val desc: String) {
    BLUR_SPLIT(0x200, "模糊分屏"),
    BLACK_WHITE_THREE(0x201, "黑白三屏"),
    TWO(0x202, "两屏"),
    THREE(0x203, "三屏"),
    FOUR(0x204, "四屏"),
    SIX(0x205, "六屏"),
    NINE(0x206, "九屏"),
    PIP(0x207, "画中画"),
    CIRCLE(0x208, "画中圆"),
}

/**
 * 水印位置
 * 0左上，1左下，2右上，3右下
 */
enum class WaterMarkLocation(val id: Int) {
    LEFT_TOP(0),
    LEFT_BOTTOM(1),
    RIGHT_TOP(2),
    RIGHT_BOTTOM(3),
}