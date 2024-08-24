
#include "FFmpegUtils.h"

#if defined(__ANDROID__)

#include <AndroidLog.h>

#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <libavutil/display.h>
#include <stdlib.h>

AVDictionary *filterCodecOptions(AVDictionary *opts, enum AVCodecID codec_id,
                                 AVFormatContext *context, AVStream *stream, AVCodec *codec) {

    AVDictionary *ret = NULL;
    AVDictionaryEntry *ret_entry = NULL;
    int flags = context->oformat ? AV_OPT_FLAG_ENCODING_PARAM : AV_OPT_FLAG_DECODING_PARAM;
    char prefix = 0;
    const AVClass *cc = avcodec_get_class();

    if (!codec) {
        codec = context->oformat ? avcodec_find_encoder(codec_id) : avcodec_find_decoder(codec_id);
    }

    switch (stream->codecpar->codec_type) {
        case AVMEDIA_TYPE_VIDEO: {
            prefix = 'v';
            flags |= AV_OPT_FLAG_VIDEO_PARAM;
            break;
        }
        case AVMEDIA_TYPE_AUDIO: {
            prefix = 'a';
            flags |= AV_OPT_FLAG_AUDIO_PARAM;
            break;
        }
        case AVMEDIA_TYPE_SUBTITLE: {
            prefix = 's';
            flags |= AV_OPT_FLAG_SUBTITLE_PARAM;
            break;
        }
        default:
            break;
    }

    while ((ret_entry = av_dict_get(opts, "", ret_entry, AV_DICT_IGNORE_SUFFIX))) {
        char *p = strchr(ret_entry->key, ':');

        /* check stream specification in opt name */
        if (p) {
            switch (checkStreamSpecifier(context, stream, p + 1)) {
                case 1: {
                    *p = 0;
                    break;
                }
                case 0:
                    continue;
                default:
                    break;
            }
        }

        if (av_opt_find(&cc, ret_entry->key, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ) ||
            !codec ||
            (codec->priv_class && av_opt_find(&codec->priv_class, ret_entry->key, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ))) {
            av_dict_set(&ret, ret_entry->key, ret_entry->value, 0);
        } else if (ret_entry->key[0] == prefix &&
                   av_opt_find(&cc, ret_entry->key + 1, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ)) {
            av_dict_set(&ret, ret_entry->key + 1, ret_entry->value, 0);
        }

        if (p) {
            *p = ':';
        }
    }
    return ret;
}

int checkStreamSpecifier(AVFormatContext *context, AVStream *stream, const char *spec) {
    int ret = avformat_match_stream_specifier(context, stream, spec);
    if (ret < 0) {
#if defined(__ANDROID__)
        LOGE("Invalid stream specifier: %s.\n", spec);
#else
        av_log(context, AV_LOG_ERROR, "Invalid stream specifier: %s.\n", spec);
#endif
    }
    return ret;
}

AVDictionary **setupStreamInfoOptions(AVFormatContext *context, AVDictionary *codec_opts) {
    int i;
    AVDictionary **opts; // 实际上是定义一个AVDictionary的数组

    if (!context->nb_streams) {
        return NULL;
    }
    // 分配一个保存 AVDictionary 指针的数组空间，opts 是这个数组空间的首地址
    opts = (AVDictionary **) av_mallocz_array(context->nb_streams, sizeof(*opts));
    if (!opts) {
#if defined(__ANDROID__)
        LOGE("Could not alloc memory for stream options.\n");
#else
        av_log(NULL, AV_LOG_ERROR, "Could not alloc memory for stream options.\n");
#endif
        return NULL;
    }
    // 遍历流
    for (i = 0; i < context->nb_streams; i++) {
        opts[i] = filterCodecOptions(codec_opts, context->streams[i]->codecpar->codec_id, context, context->streams[i], NULL);
    }
    return opts;
}

void printError(const char *file_name, int err) {
    char err_buf[128];
    const char *err_buf_ptr = err_buf;

    if (av_strerror(err, err_buf, sizeof(err_buf)) < 0) {
        err_buf_ptr = strerror(AVUNERROR(err));
    }
#if defined(__ANDROID__)
    LOGE("error  : %s \n"
         "message: %s", file_name, err_buf_ptr);
#else
    av_log(NULL, AV_LOG_ERROR, "error:%s \nmessage:%s", file_name, err_buf_ptr);
#endif
}

double getRotation(AVStream *stream) {
    AVDictionaryEntry *rotate_tag = av_dict_get(stream->metadata, "rotate", NULL, 0);
    uint8_t *displayMatrix = av_stream_get_side_data(stream, AV_PKT_DATA_DISPLAYMATRIX, NULL);
    double theta = 0;

    if (rotate_tag && *rotate_tag->value && strcmp(rotate_tag->value, "0")) {
        char *tail;
        theta = av_strtod(rotate_tag->value, &tail);
        if (*tail) {
            theta = 0;
        }
    }
    if (displayMatrix && !theta) {
        theta = -av_display_rotation_get((int32_t *) displayMatrix);
    }

    theta -= 360 * floor(theta / 360 + 0.9 / 360);

    if (fabs(theta - 90 * round(theta / 90)) > 2) {

#if defined(__ANDROID__)
        LOGW("Odd rotation angle.\n"
             "If you want to help, upload a sample "
             "of this file to ftp://upload.ffmpeg.org/incoming/ "
             "and contact the ffmpeg-devel mailing list. (ffmpeg-devel@ffmpeg.org)");
#else
        av_log(NULL, AV_LOG_WARNING, "Odd rotation angle.\n"
                "If you want to help, upload a sample "
                "of this file to ftp://upload.ffmpeg.org/incoming/ "
                "and contact the ffmpeg-devel mailing list. (ffmpeg-devel@ffmpeg.org)");
#endif
    }

    return theta;
}

int isRealTime(AVFormatContext *formatContext) {
    if (!strcmp(formatContext->iformat->name, "rtp") ||
        !strcmp(formatContext->iformat->name, "rtsp") ||
        !strcmp(formatContext->iformat->name, "sdp")) {
        return 1;
    }

    if (formatContext->pb &&
        (!strncmp(formatContext->filename, "rtp:", 4) || !strncmp(formatContext->filename, "udp:", 4))) {
        return 1;
    }
    return 0;
}


#ifdef __cplusplus
}
#endif
