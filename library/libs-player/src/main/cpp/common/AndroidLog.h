#ifndef FFMPEG4_ANDROIDLOG_H
#define FFMPEG4_ANDROIDLOG_H

#include <android/log.h>

#define ANDROID_TAG "FFmpeg4"

#if defined(__ANDROID__)

#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR, ANDROID_TAG, FORMAT, ##__VA_ARGS__)
#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO,  ANDROID_TAG, FORMAT, ##__VA_ARGS__)
#define LOGD(FORMAT, ...) __android_log_print(ANDROID_LOG_DEBUG, ANDROID_TAG, FORMAT, ##__VA_ARGS__)
#define LOGW(FORMAT, ...) __android_log_print(ANDROID_LOG_WARN,  ANDROID_TAG, FORMAT, ##__VA_ARGS__)
#define LOGV(FORMAT, ...) __android_log_print(ANDROID_LOG_VERBOSE,  ANDROID_TAG, FORMAT, ##__VA_ARGS__)

#else

#define LOGE(FORMAT, ...) {}
#define LOGI(FORMAT, ...) {}
#define LOGD(FORMAT, ...) {}
#define LOGW(FORMAT, ...) {}
#define LOGV(FORMAT, ...) {}

#endif

#endif //FFMPEG4_ANDROIDLOG_H