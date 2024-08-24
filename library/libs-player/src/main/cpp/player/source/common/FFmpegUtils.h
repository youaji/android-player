#ifndef FFMPEG4_FFMPEGUTILS_H
#define FFMPEG4_FFMPEGUTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libavutil/dict.h>
#include <libavutil/eval.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>

/**
 * @param opts
 * @param codec_id
 * @param context
 * @param stream
 * @param codec
 * @return 过滤解码器属性
 */
AVDictionary *filterCodecOptions(AVDictionary *opts, enum AVCodecID codec_id,
                             AVFormatContext *context, AVStream *stream, AVCodec *codec);

/**
* 检查媒体参数
* @param context
* @param stream
* @param spec
* @return 检查媒体流
*/
int checkStreamSpecifier(AVFormatContext *context, AVStream *stream, const char *spec);

/**
 * 设置媒体流额外参数
 * @param context
 * @param codec_opts
 * @return 设置媒体流信息
 */
AVDictionary **setupStreamInfoOptions(AVFormatContext *context, AVDictionary *codec_opts);

/**
 * 打印出错信息
 * @param file_name
 * @param err
 */
void printError(const char *file_name, int err);

/**
 * @param stream
 * @return 旋转角度
 */
double getRotation(AVStream *stream);

/**
 * @param formatContext
 * @return 是否实时流
 */
int isRealTime(AVFormatContext *formatContext);

#ifdef __cplusplus
}
#endif

#endif //FFMPEG4_FFMPEGUTILS_H
