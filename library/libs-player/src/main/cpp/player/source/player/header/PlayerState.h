#ifndef PLAYERSTATE_H
#define PLAYERSTATE_H

#include "Mutex.h"
#include "Condition.h"
#include "Thread.h"
#include "FFmpegUtils.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/mem.h"
#include "libavutil/rational.h"
#include "libavutil/time.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libavutil/avstring.h"
}

#include "AVMessageQueue.h"

#define VIDEO_QUEUE_SIZE 3
#define SAMPLE_QUEUE_SIZE 9

#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define MIN_FRAMES 25

#define AUDIO_MIN_BUFFER_SIZE 512
#define AUDIO_MAX_CALLBACKS_PER_SEC 30

#define REFRESH_RATE 0.01

#define AV_SYNC_THRESHOLD_MIN 0.04
#define AV_SYNC_THRESHOLD_MAX 0.1
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
#define AV_NOSYNC_THRESHOLD 10.0


#define EXTERNAL_CLOCK_MIN_FRAMES 2
#define EXTERNAL_CLOCK_MAX_FRAMES 10
#define EXTERNAL_CLOCK_SPEED_MIN  0.900
#define EXTERNAL_CLOCK_SPEED_MAX  1.010
#define EXTERNAL_CLOCK_SPEED_STEP 0.001
#define AUDIO_DIFF_AVG_NB   20

#define SAMPLE_CORRECTION_PERCENT_MAX 10

// Options 定义
#define OPT_CATEGORY_FORMAT 1
#define OPT_CATEGORY_CODEC 2
#define OPT_CATEGORY_SWS 3
#define OPT_CATEGORY_PLAYER 4
#define OPT_CATEGORY_SWR 5

typedef enum {
    AV_SYNC_AUDIO,      // 同步到音频时钟
    AV_SYNC_VIDEO,      // 同步到视频时钟
    AV_SYNC_EXTERNAL,   // 同步到外部时钟
} SyncType;

struct AVDictionary {
    int count;
    // 可用于配置音视频参数，此结构体是一个 key-value 的形式
    AVDictionaryEntry *elements;
};

class PlayerState {

public:
    PlayerState();

    virtual ~PlayerState();

    void reset();

    void setOption(int category, const char *type, const char *option);

    void setOptionLong(int category, const char *type, int64_t option);

private:
    void init();

    void parse_string(const char *type, const char *option);

    void parse_int(const char *type, int64_t option);

public:
    Mutex mutex;                // 操作互斥锁，主要是给 seek 操作、音视频解码以及清空解码上下文缓冲使用，不加锁会导致ffmpeg内部崩溃现象
    AVDictionary *sws_dict;     // 视频转码option参数
    AVDictionary *swr_opts;     // 音频重采样option参数
    AVDictionary *format_opts;  // 解复用option参数
    AVDictionary *codec_opts;   // 解码option参数

    AVMessageQueue *message_queue;  // 播放器消息队列
    int64_t video_duration;         // 视频时长

    AVInputFormat *input_format;    // 指定文件封装格式，也就是解复用器
    const char *url;                // 文件路径
    int64_t offset;                 // 文件偏移量
    const char *headers;            // 文件头信息

    const char *audio_codec_name;   // 指定音频解码器名称
    const char *video_codec_name;   // 指定视频解码器名称

    int abort_request;      // 退出标志
    int pause_request;      // 暂停标志
    SyncType sync_type;     // 同步类型
    int64_t start_time;     // 播放起始位置
    int64_t duration;       // 播放时长
    int real_time;          // 判断是否实时流
    int infinite_buffer;    // 是否无限缓冲区，默认为-1
    int audio_disable;      // 是否禁止音频流
    int video_disable;      // 是否禁止视频流
    int display_disable;    // 是否禁止显示

    int fast;   // 解码上下文的 AV_CODEC_FLAG2_FAST 标志
    int genpts; // 解码上下文的 AVFMT_FLAG_GENPTS 标志
    int lowres; // 解码上下文的 lowres 标志

    float playback_rate;    // 播放速度
    float playback_pitch;   // 播放音调

    int seek_by_bytes;  // 是否以字节定位
    int seek_request;   // 定位请求
    int seek_flags;     // 定位标志
    int64_t seek_pos;   // 定位位置
    int64_t seek_rel;   // 定位偏移

    int auto_exit;          // 是否自动退出
    int loop;               // 循环播放
    int mute;               // 静音播放
    int frame_drop;         // 舍帧操作
    int reorder_video_pts;  // 视频帧重排pts
};

#endif //PLAYERSTATE_H
