#include <jni.h>
#include <Mutex.h>
#include <Condition.h>
#include <Errors.h>
#include <JNIHelp.h>
#include <YouajiMediaPlayer.h>
#include <cstring>
#include <cstdio>
#include <memory.h>

extern "C" {
#include <libavcodec/jni.h>
}

const char *CLASS_NAME = "com/youaji/libs/player/YouajiPlayer";
/**
 * ===============================================================================================================
 * ===============================================================================================================
 * ===============================================================================================================
 */
struct fields_t {
    jfieldID context;       // c++ 层的 YouajiMediaPlayer 实例在java层的引用的id值
    jmethodID post_event;   // java层信息回调函数的函数ID
};
// 结构体引用，声明的时候已经分配好内存，不用手动分配和释放内存空间
static fields_t fields;

static JavaVM *javaVM = NULL;

static JNIEnv *getJNIEnv() {
    JNIEnv *env;
    // assert的作用是先计算()里面的表达式 ，若其值为假（即为0），那么它先向stderr打印一条出错信息，然后通过调用abort来终止程序运行
    assert(javaVM != NULL);
    // 因为要通过GetEnv给指针env赋值，所以用&env，指针引用就是指针的指针
    if (javaVM->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return NULL;
    }
    return env;
}

/**
 * ===============================================================================================================
 * ===============================================================================================================
 * ===============================================================================================================
 */
class JNIMediaPlayerListener : public MediaPlayerListener {
public:
    JNIMediaPlayerListener(JNIEnv *env, jobject thiz, jobject weak_thiz);

    ~JNIMediaPlayerListener();

    //后面用override，代表重写基类的虚函数
    void notify(int msg, int ext1, int ext2, void *obj) override;

private:
    //将无参构造函数设置为私有，不被外界调用
    JNIMediaPlayerListener() {}

    jclass mClass; //java层easymediaplayer类的class实例
    //cpp层对java层的easymediaplayer实例的弱引用，因为是弱引用，所以当java层的实例要销毁即不再有强引用的时候，这个弱引用不会造成内存泄漏
    //回调消息通知notify方法的时候，传递这个弱引用给java层，在java层再通过调用对应的方法通知消息
    jobject mObject;
};

// 定义构造函数
JNIMediaPlayerListener::JNIMediaPlayerListener(JNIEnv *env, jobject thiz, jobject weak_thiz) {
    jclass clazz = env->GetObjectClass(thiz);
    if (clazz == NULL) {
        LOGE("Can't find %s", CLASS_NAME);
        return;
    }
    // 创建一个全局引用，不用时必须调用 DeleteGlobalRef() 方法释放，参数为某个Java 对象实例，可以是局部引用或全局引用
    mClass = (jclass) env->NewGlobalRef(clazz);

    // We use a weak reference so the MediaPlayer object can be garbage collected
    // The reference is only used as a proxy for callbacks
    mObject = env->NewGlobalRef(weak_thiz);
}

//定义析构函数
JNIMediaPlayerListener::~JNIMediaPlayerListener() {
    JNIEnv *env = getJNIEnv();
    //释放全局引用
    env->DeleteGlobalRef(mObject);
    env->DeleteGlobalRef(mClass);
}

//ext1和ext2对应msg的arg1和arg2，即仅传递低成本的int值，obj代表传递一个对象
void JNIMediaPlayerListener::notify(int msg, int ext1, int ext2, void *obj) {
    JNIEnv *env = getJNIEnv();
    //获取对应线程的JNIEnv对象，将当前线程跟JV、M关联
    bool status = (javaVM->AttachCurrentThread(&env, NULL) >= 0);

//    LOGI("notify msg:%d ext1:%d ext2:%d", msg, ext1, ext2);
    // TODO obj needs changing into jobject
    // 调用java层的static方法，mObject, msg, ext1, ext2, obj 均为方法参数，对应java层handler的方法obtainMessage(what, arg1, arg2, obj)，msg为消息ID
    // 调用java层的static方法，关键需要java层对应类的class对象和方法ID，后面就是参数
    env->CallStaticVoidMethod(mClass, fields.post_event, mObject, msg, ext1, ext2, obj);

    if (env->ExceptionCheck()) {
        LOGW("An exception occurred while notifying an event.");
        env->ExceptionClear();
    }
    //记得解除链接
    if (status) {
        javaVM->DetachCurrentThread();
    }
}

/**
 * ===============================================================================================================
 * ===============================================================================================================
 * ===============================================================================================================
 */
static YouajiMediaPlayer *getMediaPlayer(JNIEnv *env, jobject thiz) {
    YouajiMediaPlayer *const mp = (YouajiMediaPlayer *) env->GetLongField(thiz, fields.context);
    return mp;
}

static YouajiMediaPlayer *setMediaPlayer(JNIEnv *env, jobject thiz, long mediaPlayer) {
    YouajiMediaPlayer *old = (YouajiMediaPlayer *) env->GetLongField(thiz, fields.context);
    env->SetLongField(thiz, fields.context, mediaPlayer);
    return old;
}

// If exception is NULL and opStatus is not OK, this method sends an error
// event to the client application; otherwise, if exception is not NULL and
// opStatus is not OK, this method throws the given exception to the client
// application.
static void process_media_player_call(JNIEnv *env, jobject thiz, int opStatus, const char *exception, const char *message) {
    if (exception == NULL) {  // Don't throw exception. Instead, send an event.
        if (opStatus != (int) OK) {
            YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
            if (mp != 0) mp->notify(MEDIA_ERROR, opStatus, 0);
        }
    } else {
        if (opStatus == (int) INVALID_OPERATION) {
            jniThrowException(env, "java/lang/IllegalStateException");
        } else if (opStatus == (int) PERMISSION_DENIED) {
            jniThrowException(env, "java/lang/SecurityException");
        } else if (opStatus != (int) OK) {
            if (strlen(message) > 230) {
                jniThrowException(env, exception, message);
            } else {
                char msg[256];
                sprintf(msg, "%s: status=0x%X", message, opStatus);
                jniThrowException(env, exception, msg);
            }
        }
    }
}

/**
* ===============================================================================================================
* ===============================================================================================================
* ===============================================================================================================
*/
void Player_native_init(JNIEnv *env, jclass thiz) {
    jclass clazz = env->FindClass(CLASS_NAME);
    if (clazz == NULL) {
        return;
    }

    fields.context = env->GetFieldID(clazz, "nativeContext", "J");
    if (fields.context == NULL) {
        return;
    }

    fields.post_event = env->GetStaticMethodID(clazz, "nativePostEvent", "(Ljava/lang/Object;IIILjava/lang/Object;)V");
    if (fields.post_event == NULL) {
        return;
    }

    env->DeleteLocalRef(clazz);
}

void Player_native_setup(JNIEnv *env, jobject thiz, jobject mediaPlayerObj) {
    YouajiMediaPlayer *mp = new YouajiMediaPlayer();

    mp->init();

    // create new listener and give it to MediaPlayer
    JNIMediaPlayerListener *listener = new JNIMediaPlayerListener(env, thiz, mediaPlayerObj);
    // 设置监听
    mp->setListener(listener);

    // Stow our new C++ MediaPlayer in an opaque field in the Java object.其中opaque field 不透明的变量，即私有变量
    setMediaPlayer(env, thiz, (long) mp);
}

void Player_nativeSetSurface(JNIEnv *env, jobject thiz, jobject surface) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        return;
    }
    ANativeWindow *window = NULL;
    if (surface != NULL) {
        window = ANativeWindow_fromSurface(env, surface);
    }
    mp->setVideoSurface(window);
}

void Player_nativeSurfaceChanged(JNIEnv *env, jobject thiz, jint width, jint height) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        return;
    }
    mp->surfaceChanged(width, height);
}

void Player_nativeSetDataSource_fileDescriptor(JNIEnv *env, jobject thiz, jobject fileDescriptor, jlong offset, jlong length) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        return;
    }

    if (fileDescriptor == NULL) {
        return;
    }

    int fd = jniGetFDFromFileDescriptor(env, fileDescriptor);
    if (offset < 0 || length < 0 || fd < 0) {
        if (offset < 0) {
            LOGE("negative offset (%lld)", offset);
        }
        if (length < 0) {
            LOGE("negative length (%lld)", length);
        }
        if (fd < 0) {
            LOGE("invalid file descriptor");
        }
        return;
    }

    char path[256] = "";
    int myfd = dup(fd);
    char str[20];
    sprintf(str, "pipe:%d", myfd);
    strcat(path, str);

    status_t opStatus = mp->setDataSource(path, offset, NULL);
    process_media_player_call(env, thiz, opStatus, "java/io/IOException", "setDataSourceFD failed.");
}

void Player_nativeSetDataSource_header(JNIEnv *env, jobject thiz, jstring _path, jobjectArray keys, jobjectArray values) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        return;
    }

    if (_path == NULL) {
        return;
    }
    //将 Java 风格的 jstring 对象转换成 C 风格的字符串，0为不拷贝
    const char *path = env->GetStringUTFChars(_path, 0);
    if (path == NULL) {
        return;
    }
    //MMS协议：MMS（MicrosoftMediaServerprotocol）是一种串流媒体传送协议，用来访问并流式接收Windows Media服务器中.asf文件的一种协议
    //strstr(str1,str2) 函数用于判断字符串str2是否是str1的子串。如果是，则该函数返回str1字符串从str2第一次出现的位置开始到str1结尾的字符串；否则，返回NULL
    const char *restrict = strstr(path, "mms://"); //是否为mms协议的多媒体
    //strdup是复制字符串，返回一个指针,指向为复制字符串分配的空间;如果分配空间失败,则返回NULL值
    char *restrict_to = restrict ? strdup(restrict) : NULL;
    if (restrict_to != NULL) {
        //把src所指向的字符串复制到dest，最多复制n个字符。当src的长度小于n时，dest的剩余部分将用空字节填充
        strncpy(restrict_to, "mmsh://", 6);
        //将path输出到屏幕
        puts(path);
    }

    char *headers = NULL;
    if (keys && values != NULL) {
        int keysCount = env->GetArrayLength(keys);
        int valuesCount = env->GetArrayLength(values);

        if (keysCount != valuesCount) {
            LOGE("keys and values arrays have different length");
            return;
        }

        const char *rawString = NULL;
        char hdrs[2048];

        for (int i = 0; i < keysCount; i++) {
            jstring key = (jstring) env->GetObjectArrayElement(keys, i);
            rawString = env->GetStringUTFChars(key, NULL);
            //char *strcat(char *dest, const char *src) 把src所指向的字符串追加到dest所指向的字符串的结尾
            strcat(hdrs, rawString);
            strcat(hdrs, ": ");
            env->ReleaseStringUTFChars(key, rawString);

            jstring value = (jstring) env->GetObjectArrayElement(values, i);
            rawString = env->GetStringUTFChars(value, NULL);
            strcat(hdrs, rawString);
            strcat(hdrs, "\r\n"); //回车换行
            env->ReleaseStringUTFChars(value, rawString);
        }

        headers = &hdrs[0];
    }

    status_t opStatus = mp->setDataSource(path, 0, headers);
    process_media_player_call(env, thiz, opStatus, "java/io/IOException", "setDataSource failed.");

    env->ReleaseStringUTFChars(_path, path);
}

void Player_nativeSetDataSource(JNIEnv *env, jobject thiz, jstring _path) {
    Player_nativeSetDataSource_header(env, thiz, _path, NULL, NULL);
}

void Player_nativePrepare(JNIEnv *env, jobject thiz) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        return;
    }

    mp->prepare();
}

void Player_nativePrepareAsync(JNIEnv *env, jobject thiz) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        return;
    }
    mp->prepareAsync();
}

void Player_nativeStart(JNIEnv *env, jobject thiz) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        LOGE("MediaPlayer is null");
        return;
    }
    mp->start();
}

void Player_nativeStop(JNIEnv *env, jobject thiz) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        LOGE("MediaPlayer is null");
        return;
    }
    mp->stop();
}

void Player_nativePause(JNIEnv *env, jobject thiz) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        return;
    }
    mp->pause();
}

void Player_nativeResume(JNIEnv *env, jobject thiz) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        return;
    }
    mp->resume();
}

void Player_nativeRelease(JNIEnv *env, jobject thiz) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp != nullptr) {
        mp->disconnect();
        delete mp;
        setMediaPlayer(env, thiz, 0);
    }
}

void Player_nativeReset(JNIEnv *env, jobject thiz) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        return;
    }
    mp->reset();
}

void Player_nativeFinalize(JNIEnv *env, jobject thiz) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp != NULL) {
        LOGW("MediaPlayer finalized without being released");
    }
    LOGD("调用完成");
    Player_nativeRelease(env, thiz);
}

void Player_nativeSeekTo(JNIEnv *env, jobject thiz, jfloat millisecond) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        return;
    }
    mp->seekTo(millisecond);
}

jboolean Player_nativeIsPlaying(JNIEnv *env, jobject thiz) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return JNI_FALSE;
    }
    return (jboolean) (mp->isPlaying() ? JNI_TRUE : JNI_FALSE);
}

void Player_nativeSetStereoVolume(JNIEnv *env, jobject thiz, jint leftPercent, jint rightPercent) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }
    mp->setStereoVolume(leftPercent, rightPercent);
}

void Player_nativeSetVolume(JNIEnv *env, jobject thiz, jint percent) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }
    mp->setVolume(percent);
}

jfloat Player_nativeGetVolume(JNIEnv *env, jobject thiz) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return 0;
    }
    return mp->getVolume();
}

void Player_nativeSetLooping(JNIEnv *env, jobject thiz, jboolean looping) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }
    mp->setLooping(looping);
}

jboolean Player_nativeGetLooping(JNIEnv *env, jobject thiz) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return JNI_FALSE;
    }
    return (jboolean) (mp->isLooping() ? JNI_TRUE : JNI_FALSE);
}

void Player_nativeSetMute(JNIEnv *env, jobject thiz, jboolean mute) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }
    mp->setMute(mute);
}

void Player_nativeSetPitch(JNIEnv *env, jobject thiz, jfloat pitch) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }
    mp->setPitch(pitch);
}

void Player_nativeSetRate(JNIEnv *env, jobject thiz, jfloat rate) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }
    mp->setRate(rate);
}

jint Player_nativeGetRotate(JNIEnv *env, jobject thiz) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return 0;
    }
    return mp->getRotate();
}

jlong Player_nativeGetDuration(JNIEnv *env, jobject thiz) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return 0L;
    }
    return mp->getDuration();
}

jint Player_nativeGetVideoWidth(JNIEnv *env, jobject thiz) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return 0;
    }
    return mp->getVideoWidth();
}

jint Player_nativeGetVideoHeight(JNIEnv *env, jobject thiz) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return 0;
    }
    return mp->getVideoHeight();
}

jlong Player_nativeGetPosition(JNIEnv *env, jobject thiz) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return 0L;
    }
    return mp->getCurrentPosition();
}

void Player_nativeChangeFilterByName(JNIEnv *env, jobject thiz, int node_type, jstring filterName) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }

    const char *name = env->GetStringUTFChars(filterName, NULL);
    mp->changeFilter(node_type, name);
    env->ReleaseStringUTFChars(filterName, name);
}

void Player_nativeChangeFilterById(JNIEnv *env, jobject thiz, int node_type, jint filterId) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }

    mp->changeFilter(node_type, filterId);
}

void Player_nativeSetWatermark(JNIEnv *env, jobject thiz, jbyteArray dataArray, jint dataLen, jint waterMarkWidth, jint waterMarkHeight, jfloat scale, jint location) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }

    jbyte *data = env->GetByteArrayElements(dataArray, NULL);
    mp->setWatermark((uint8_t *) data, (size_t) dataLen, waterMarkWidth, waterMarkHeight, scale, location);
    env->ReleaseByteArrayElements(dataArray, data, 0);
}

jboolean Player_nativeStartRecord(JNIEnv *env, jobject thiz, jstring filePath) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return JNI_FALSE;
    }

    const char *file = env->GetStringUTFChars(filePath, nullptr);
    jboolean result = mp->startRecord(file) ? JNI_TRUE : JNI_FALSE;
    env->ReleaseStringUTFChars(filePath, file);
    return result;
}

jboolean Player_nativeStopRecord(JNIEnv *env, jobject thiz) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return JNI_FALSE;
    }
    return (jboolean) (mp->stopRecord() ? JNI_TRUE : JNI_FALSE);
}

jboolean Player_nativeIsRecording(JNIEnv *env, jobject thiz) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return JNI_FALSE;
    }
    return (jboolean) (mp->isRecording() ? JNI_TRUE : JNI_FALSE);
}

jboolean Player_nativeScreenshot(JNIEnv *env, jobject thiz, jstring filePath) {
    YouajiMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return JNI_FALSE;
    }
    const char *file = env->GetStringUTFChars(filePath, nullptr);
    env->ReleaseStringUTFChars(filePath, file);
    return (jboolean) (mp->screenshot(file) ? JNI_TRUE : JNI_FALSE);
}

/**
 * ===============================================================================================================
 * ===============================================================================================================
 * ===============================================================================================================
 */
static const JNINativeMethod gMethods[] = {
        {"nativeInit",               "()V",                                                         (void *) Player_native_init},
        {"nativeSetup",              "(Ljava/lang/Object;)V",                                       (void *) Player_native_setup},
        {"nativeSetSurface",         "(Landroid/view/Surface;)V",                                   (void *) Player_nativeSetSurface},
        {"nativeSurfaceChanged",     "(II)V",                                                       (void *) Player_nativeSurfaceChanged},
        {"nativeSetDataSource",      "(Ljava/io/FileDescriptor;JJ)V",                               (void *) Player_nativeSetDataSource_fileDescriptor},
        {"nativeSetDataSource",      "(Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/String;)V", (void *) Player_nativeSetDataSource_header},
        {"nativeSetDataSource",      "(Ljava/lang/String;)V",                                       (void *) Player_nativeSetDataSource},
        {"nativePrepare",            "()V",                                                         (void *) Player_nativePrepare},
        {"nativePrepareAsync",       "()V",                                                         (void *) Player_nativePrepareAsync},
        {"nativeStart",              "()V",                                                         (void *) Player_nativeStart},
        {"nativeStop",               "()V",                                                         (void *) Player_nativeStop},
        {"nativePause",              "()V",                                                         (void *) Player_nativePause},
        {"nativeResume",             "()V",                                                         (void *) Player_nativeResume},
        {"nativeRelease",            "()V",                                                         (void *) Player_nativeRelease},
        {"nativeReset",              "()V",                                                         (void *) Player_nativeReset},
        {"nativeFinalize",           "()V",                                                         (void *) Player_nativeFinalize},
        {"nativeSeekTo",             "(F)V",                                                        (void *) Player_nativeSeekTo},
        {"nativeIsPlaying",          "()Z",                                                         (void *) Player_nativeIsPlaying},
        {"nativeSetStereoVolume",    "(II)V",                                                       (void *) Player_nativeSetStereoVolume},
        {"nativeSetVolume",          "(I)V",                                                        (void *) Player_nativeSetVolume},
        {"nativeGetVolume",          "()F",                                                         (void *) Player_nativeGetVolume},
        {"nativeSetLooping",         "(Z)V",                                                        (void *) Player_nativeSetLooping},
        {"nativeGetLooping",         "()Z",                                                         (void *) Player_nativeGetLooping},
        {"nativeSetMute",            "(Z)V",                                                        (void *) Player_nativeSetMute},
        {"nativeSetPitch",           "(F)V",                                                        (void *) Player_nativeSetPitch},
        {"nativeSetRate",            "(F)V",                                                        (void *) Player_nativeSetRate},
        {"nativeGetRotate",          "()I",                                                         (void *) Player_nativeGetRotate},
        {"nativeGetDuration",        "()J",                                                         (void *) Player_nativeGetDuration},
        {"nativeGetVideoWidth",      "()I",                                                         (void *) Player_nativeGetVideoWidth},
        {"nativeGetVideoHeight",     "()I",                                                         (void *) Player_nativeGetVideoHeight},
        {"nativeGetPosition",        "()J",                                                         (void *) Player_nativeGetPosition},
        {"nativeChangeFilterById",   "(II)V",                                                       (void *) Player_nativeChangeFilterById},
        {"nativeChangeFilterByName", "(ILjava/lang/String;)V",                                      (void *) Player_nativeChangeFilterByName},
        {"nativeSetWaterMark",       "([BIIIFI)V",                                                  (void *) Player_nativeSetWatermark},
        {"nativeStartRecord",        "(Ljava/lang/String;)Z",                                       (void *) Player_nativeStartRecord},
        {"nativeStopRecord",         "()Z",                                                         (void *) Player_nativeStopRecord},
        {"nativeIsRecording",        "()Z",                                                         (void *) Player_nativeIsRecording},
        {"nativeScreenshot",         "(Ljava/lang/String;)Z",                                       (void *) Player_nativeScreenshot},

};

// 注册YouajiMediaPlayer的Native方法
static int register_com_youaji_libs_player_YouajiPlayer(JNIEnv *env) {
    int numMethods = (sizeof(gMethods) / sizeof((gMethods)[0]));
    jclass clazz = env->FindClass(CLASS_NAME);
    if (clazz == NULL) {
        LOGE("Native registration unable to find class '%s'", CLASS_NAME);
        return JNI_ERR;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        LOGE("Native registration unable to find class '%s'", CLASS_NAME);
        return JNI_ERR;
    }
    env->DeleteLocalRef(clazz);

    return JNI_OK;
}

extern "C"
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    av_jni_set_java_vm(vm, NULL);
    javaVM = vm;
    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }
    if (register_com_youaji_libs_player_YouajiPlayer(env) != JNI_OK) {
        return -1;
    }
    return JNI_VERSION_1_4;
}
