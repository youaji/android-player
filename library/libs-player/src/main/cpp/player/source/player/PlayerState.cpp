#include <AndroidLog.h>
#include "PlayerState.h"

PlayerState::PlayerState() {
    init();
    reset();
}

PlayerState::~PlayerState() {
    reset();
    if (message_queue) {
        message_queue->release();
        delete message_queue;
        message_queue = nullptr;
    }
}

void PlayerState::init() {
    sws_dict = (AVDictionary *) malloc(sizeof(AVDictionary));
    memset(sws_dict, 0, sizeof(AVDictionary));
    swr_opts = (AVDictionary *) malloc(sizeof(AVDictionary));
    memset(swr_opts, 0, sizeof(AVDictionary));
    format_opts = (AVDictionary *) malloc(sizeof(AVDictionary));
    memset(format_opts, 0, sizeof(AVDictionary));
    codec_opts = (AVDictionary *) malloc(sizeof(AVDictionary));
    memset(codec_opts, 0, sizeof(AVDictionary));

    input_format = NULL;
    url = NULL;
    headers = NULL;

    audio_codec_name = NULL;
    video_codec_name = NULL;
    message_queue = new AVMessageQueue();
}

void PlayerState::reset() {

    if (sws_dict) {
        av_dict_free(&sws_dict);
        av_dict_set(&sws_dict, "flags", "bicubic", 0);
    }
    if (swr_opts) {
        av_dict_free(&swr_opts);
    }
    if (format_opts) {
        av_dict_free(&format_opts);
    }
    if (codec_opts) {
        av_dict_free(&codec_opts);
    }
    if (url) {
        av_freep(&url);
        url = NULL;
    }
    offset = 0;
    abort_request = 1;
    LOGD("PlayerState->重置播放状态 --- 如设置暂停标志等");
    pause_request = 1;
    seek_by_bytes = 0;
    sync_type = AV_SYNC_AUDIO; //以音频时钟为准
    start_time = AV_NOPTS_VALUE;
    duration = AV_NOPTS_VALUE;
    real_time = 0;
    infinite_buffer = -1;
    audio_disable = 0;
    video_disable = 0;
    display_disable = 0;
    fast = 0;
    genpts = 0;
    lowres = 0;
    playback_rate = 1.0;
    playback_pitch = 1.0;
    seek_request = 0;
    seek_flags = 0;
    seek_pos = 0;
    seek_rel = 0;
    seek_rel = 0;
    auto_exit = 0;
    loop = 0;
    mute = 0;
    frame_drop = 1;
    reorder_video_pts = 1;
    video_duration = 0;
}

void PlayerState::setOption(int category, const char *type, const char *option) {
    switch (category) {
        case OPT_CATEGORY_FORMAT: {
            av_dict_set(&format_opts, type, option, 0);
            break;
        }

        case OPT_CATEGORY_CODEC: {
            av_dict_set(&codec_opts, type, option, 0);
            break;
        }

        case OPT_CATEGORY_SWS: {
            av_dict_set(&sws_dict, type, option, 0);
            break;
        }

        // 播放器参数
        case OPT_CATEGORY_PLAYER: {
            parse_string(type, option);
            break;
        }

        case OPT_CATEGORY_SWR: {
            av_dict_set(&swr_opts, type, option, 0);
            break;
        }
    }
}

void PlayerState::setOptionLong(int category, const char *type, int64_t option) {
    switch (category) {
        case OPT_CATEGORY_FORMAT: {
            av_dict_set_int(&format_opts, type, option, 0);
            break;
        }

        case OPT_CATEGORY_CODEC: {
            av_dict_set_int(&codec_opts, type, option, 0);
            break;
        }

        case OPT_CATEGORY_SWS: {
            av_dict_set_int(&sws_dict, type, option, 0);
            break;
        }

        case OPT_CATEGORY_PLAYER: {
            parse_int(type, option);
            break;
        }

        case OPT_CATEGORY_SWR: {
            av_dict_set_int(&swr_opts, type, option, 0);
            break;
        }
    }
}

void PlayerState::parse_string(const char *type, const char *option) {
    //对两个字符串自左至右逐个字符相比（按ASCII码值大小比较），直到出现不同的字符或遇到‘\0’为止
    if (!strcmp("acodec", type)) { // 指定音频解码器名称
        audio_codec_name = av_strdup(option);
    } else if (!strcmp("vcodec", type)) {   // 指定视频解码器名称
        video_codec_name = av_strdup(option);
    } else if (!strcmp("sync", type)) { // 制定同步类型
        if (!strcmp("audio", option)) {
            sync_type = AV_SYNC_AUDIO;
        } else if (!strcmp("video", option)) {
            sync_type = AV_SYNC_VIDEO;
        } else if (!strcmp("ext", option)) {
            sync_type = AV_SYNC_EXTERNAL;
        } else {    // 其他则使用默认的音频同步
            sync_type = AV_SYNC_AUDIO;
        }
    } else if (!strcmp("f", type)) { // f 指定输入文件格式
        input_format = av_find_input_format(option);
        if (!input_format) {
            av_log(NULL, AV_LOG_FATAL, "Unknown input format: %s\n", option);
        }
    }
}

void PlayerState::parse_int(const char *type, int64_t option) {
    if (!strcmp("an", type)) { // 禁用音频
        audio_disable = (option != 0) ? 1 : 0;
    } else if (!strcmp("vn", type)) { // 禁用视频
        video_disable = (option != 0) ? 1 : 0;
    } else if (!strcmp("bytes", type)) { // 以字节方式定位
        seek_by_bytes = (option > 0) ? 1 : ((option < 0) ? -1 : 0);
    } else if (!strcmp("nodisp", type)) { // 不显示
        display_disable = (option != 0) ? 1 : 0;
    } else if (!strcmp("fast", type)) { // fast标志
        fast = (option != 0) ? 1 : 0;
    } else if (!strcmp("genpts", type)) { // genpts标志
        genpts = (option != 0) ? 1 : 0;
    } else if (!strcmp("lowres", type)) { // lowres标准字
        lowres = (option != 0) ? 1 : 0;
    } else if (!strcmp("drp", type)) { // 重排pts
        reorder_video_pts = (option != 0) ? 1 : 0;
    } else if (!strcmp("autoexit", type)) { // 自动退出标志
        auto_exit = (option != 0) ? 1 : 0;
    } else if (!strcmp("framedrop", type)) { // 丢帧标志
        frame_drop = (option != 0) ? 1 : 0;
    } else if (!strcmp("infbuf", type)) { // 无限缓冲区标志
        infinite_buffer = (option > 0) ? 1 : ((option < 0) ? -1 : 0);
    } else {
        LOGE("unknown option - '%s'", type);
    }
}
