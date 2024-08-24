package com.youaji.libs.player

import android.annotation.SuppressLint
import android.content.Context
import android.content.res.AssetFileDescriptor
import android.graphics.Bitmap
import android.net.Uri
import android.os.Handler
import android.os.Looper
import android.os.Message
import android.util.Log
import android.view.Surface
import android.view.TextureView
import java.io.FileDescriptor
import java.lang.ref.WeakReference

open class YouajiPlayer : BasicMediaPlayer() {
    companion object {

        init {
            System.loadLibrary("soundtouch")
            System.loadLibrary("player")
            nativeInit()
        }

        @JvmStatic
        private external fun nativeInit()

        @JvmStatic
        private fun nativePostEvent(mediaPlayerRef: Any, what: Int, arg1: Int, arg2: Int, obj: Any) {
            val mp = (mediaPlayerRef as WeakReference<*>).get() as YouajiPlayer? ?: return
            mp.eventHandler?.let { handler ->
                val m = handler.obtainMessage(what, arg1, arg2, obj)
                handler.sendMessage(m)
            }
        }
    }

    private var eventHandler: EventHandler? = null

    // 渲染结点类型，跟Native层[NodeType.h RenderNodeType]数值保持一致。
    private enum class RenderNodeType(val id: Int) {
        NODE_NONE(-1),    // 未知结点
        NODE_INPUT(0),    // 输入结点
        NODE_BEAUTY(1),   // 美颜结点
        NODE_FACE(2),     // 美型结点
        NODE_MAKEUP(3),   // 彩妆结点
        NODE_FILTER(4),   // 滤镜结点
        NODE_EFFECT(5),   // 特效结点
        NODE_STICKERS(6), // 贴纸结点
        NODE_DISPLAY(7),  // 显示结点
    }

    init {
        Looper.myLooper()?.let {
            eventHandler = EventHandler(this, it)
        } ?: Looper.getMainLooper()?.let {
            eventHandler = EventHandler(this, it)
        }
        nativeSetup(WeakReference(this))
    }

    override fun setTextureView(textureView: TextureView) {}

    override fun play(path: String) {}
    override fun play(uri: Uri) {}

    override fun repeatPlay() {}

    override fun changeBackground(background: Background) {
        nativeChangeFilterByName(RenderNodeType.NODE_BEAUTY.id, background.desc)
    }

    override fun changeEffect(effect: Effect) {
        nativeChangeFilterByName(RenderNodeType.NODE_EFFECT.id, effect.desc)
    }

    override fun changeFilter(filter: Filter) {
        nativeChangeFilterByName(RenderNodeType.NODE_FILTER.id, filter.desc)
    }

    override fun setWaterMark(dataArray: ByteArray, dataLen: Int, width: Int, height: Int, scale: Float, location: WaterMarkLocation) {
        nativeSetWaterMark(dataArray, dataLen, width, height, scale, location.id)
    }

    override fun setSurface(surface: Surface) {
        nativeSetSurface(surface)
    }

    override fun setDataSource(path: String) {
        setDataSource(path, null)
    }

    override fun setDataSource(path: String, headers: Map<String, String>?) {
        headers?.let {
            if (headers.isNotEmpty()) {
                nativeSetDataSource(path, headers.keys.toTypedArray(), headers.values.toTypedArray())
            } else {
                nativeSetDataSource(path)
            }
        } ?: nativeSetDataSource(path)
    }

    override fun setDataSource(context: Context, uri: Uri) {
        setDataSource(context, uri, null)
    }

    override fun setDataSource(context: Context, uri: Uri, headers: Map<String, String>?) {
        if (uri.scheme?.equals("file") == true) {
            uri.path?.let { path -> setDataSource(path) }
            return
        }

        var fileDescriptor: AssetFileDescriptor? = null
        try {
            val contentResolver = context.contentResolver
            fileDescriptor = contentResolver.openAssetFileDescriptor(uri, "r")
            fileDescriptor?.let { fileDesc ->
                if (fileDesc.declaredLength < 0) {
                    setDataSource(fileDesc.fileDescriptor)
                } else {
                    setDataSource(fileDesc.fileDescriptor, fileDesc.startOffset, fileDesc.declaredLength)
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        } finally {
            fileDescriptor?.close()
        }

        setDataSource(uri.toString(), headers)
    }

    override fun setDataSource(fileDescriptor: FileDescriptor) {
        setDataSource(fileDescriptor, 0, 0x7ffffffffffffffL)
    }

    override fun setDataSource(fileDescriptor: FileDescriptor, offset: Long, length: Long) {
        nativeSetDataSource(fileDescriptor, offset, length)
    }

    override fun prepare() {
        nativePrepare()
    }

    override fun prepareAsync() {
        nativePrepareAsync()
    }

    override fun start() {
        nativeStart()
    }

    override fun stop() {
        nativeStop()
    }

    override fun pause() {
        nativePause()
    }

    override fun resume() {
        nativeResume()
    }

    override fun setLooping(looping: Boolean) {
        nativeSetLooping(looping)
    }

    override fun isLooping(): Boolean {
        return nativeGetLooping()
    }

    override fun isPlaying(): Boolean {
        return nativeIsPlaying()
    }

    override fun seekTo(millisecond: Float) {
        nativeSeekTo(millisecond)
    }

    override fun getDuration(): Long {
        return nativeGetDuration()
    }

    override fun getPosition(): Long {
        return nativeGetPosition()
    }

    override fun getRotate(): Int {
        return nativeGetRotate()
    }

    override fun getVideoWidth(): Int {
        return nativeGetVideoWidth()
    }

    override fun getVideoHeight(): Int {
        return nativeGetVideoHeight()
    }

    override fun setMute(mute: Boolean) {
        nativeSetMute(mute)
    }

    override fun setStereoVolume(leftPercent: Int, rightPercent: Int) {
        nativeSetStereoVolume(leftPercent, rightPercent)
    }

    override fun setVolume(percent: Int) {
        nativeSetVolume(percent)
    }

    override fun getVolume(): Int {
        return nativeGetVolume().toInt()
    }

    override fun surfaceChange(width: Int, height: Int) {
        nativeSurfaceChanged(width, height)
    }

    override fun setRate(rate: Float) {
        nativeSetRate(rate)
    }

    override fun setPitch(pitch: Float) {
        nativeSetPitch(pitch)
    }

    override fun startRecord(filePath: String): Boolean {
        return nativeStartRecord(filePath)
    }

    override fun stopRecord(): Boolean {
        return nativeStopRecord()
    }

    override fun isRecording(): Boolean {
        return nativeIsRecording()
    }

    override fun screenshotBitmap(): Bitmap? {
        return null
    }

    override fun screenshot(filePath: String): Boolean {
        return nativeScreenshot(filePath)
    }

    override fun release() {
        setOnPreparedListener(null)
        setOnVideoSizeChangedListener(null)
        setOnLoadingListener(null)
        setOnTimeUpdateListener(null)
        setOnErrorListener(null)
        setOnCompleteListener(null)
        setOnInfoListener(null)
//        nativeFinalize()
        nativeRelease()
    }

    fun reset() {
        nativeReset()
        eventHandler?.removeCallbacksAndMessages(null)
    }

    private var nativeContext: Long = 0 //对应native层的EMediaPlayer对象

    private external fun nativeSetup(mediaPlayer: Any)
    private external fun nativeSetSurface(surface: Surface)
    private external fun nativeSurfaceChanged(width: Int, height: Int)
    private external fun nativeSetDataSource(fd: FileDescriptor, offset: Long, length: Long)
    private external fun nativeSetDataSource(path: String, keys: Array<String>, values: Array<String>)
    private external fun nativeSetDataSource(path: String)
    private external fun nativePrepare()
    private external fun nativePrepareAsync()
    private external fun nativeStart()
    private external fun nativeStop()
    private external fun nativePause()
    private external fun nativeResume()
    private external fun nativeRelease()
    private external fun nativeReset()
    private external fun nativeFinalize()
    private external fun nativeSeekTo(millisecond: Float)
    private external fun nativeIsPlaying(): Boolean
    private external fun nativeSetStereoVolume(leftPercent: Int, rightPercent: Int)
    private external fun nativeSetVolume(percent: Int)
    private external fun nativeGetVolume(): Float
    private external fun nativeSetLooping(looping: Boolean)
    private external fun nativeGetLooping(): Boolean
    private external fun nativeSetMute(mute: Boolean)
    private external fun nativeSetPitch(pitch: Float)
    private external fun nativeSetRate(rate: Float)
    private external fun nativeGetRotate(): Int
    private external fun nativeGetDuration(): Long
    private external fun nativeGetVideoWidth(): Int
    private external fun nativeGetVideoHeight(): Int
    private external fun nativeGetPosition(): Long
    private external fun nativeChangeFilterById(type: Int, id: Int)
    private external fun nativeChangeFilterByName(type: Int, name: String)
    private external fun nativeSetWaterMark(dataArray: ByteArray, dataLen: Int, width: Int, height: Int, scale: Float, location: Int)
    private external fun nativeStartRecord(filePath: String): Boolean
    private external fun nativeStopRecord(): Boolean
    private external fun nativeIsRecording(): Boolean
    private external fun nativeScreenshot(filePath: String): Boolean

//    /** 准备状态  由native层回调 */
//    fun onPreparedNative() {
//        preparedListener?.onPrepared(this)
//    }
//
//    /**
//     * 视频尺寸回调  由native层回调
//     * @param width  宽
//     * @param height 高
//     * @param dar    比例
//     */
//    fun onVideoSizeChangedNative(width: Int, height: Int, dar: Float) {
//        videoSizeChangedListener?.onVideoSizeChanged(this, width, height, dar)
//    }
//
//    /**
//     * 加载状态 由native层回调
//     * @param load
//     */
//    fun onLoadingNative(load: Boolean) {
//        loadingListener?.onLoading(this, load)
//    }
//
//    /**
//     * 时间更新 由native层回调
//     * @param currentTime
//     * @param totalTime
//     */
//    fun onTimeUpdateNative(currentTime: Int, totalTime: Int) {
//        videoDuration = totalTime
//        timeUpdateListener?.onTimeUpdate(this, currentTime, totalTime)
//    }
//
//    /**
//     * 错误回调 由native层回调
//     * @param code -
//     * @param message  -
//     */
//    fun onErrorNative(code: Int, message: String) {
//        errorListener?.onError(this, code, message)
//    }
//
//    /** 播放完成 由native层回调 */
//    fun onCompletionNative() {
//        completionListener?.onCompletion(this)
//        if (isLooping()) { //循环状态，则延迟重播
////            mTimeDisposable = Flowable.timer(500, TimeUnit.MILLISECONDS)
////                .subscribeOn(Schedulers.io())
////                .observeOn(AndroidSchedulers.mainThread())
////                .subscribe(object : Consumer<Long?>() {
////                    @Throws(Exception::class)
////                    fun accept(aLong: Long?) {
////                        play(path, looping)
////                    }
////                })
////            mCompositeDisposable.add(mTimeDisposable)
//        }
//    }


    @SuppressLint("WakelockTimeout")
    private fun stayAwake(awake: Boolean) {
//        if (wakeLock != null) {
//            if (awake && wakeLock?.isHeld == false) {
//                wakeLock?.acquire()
//            } else if (!awake && wakeLock?.isHeld == true) {
//                wakeLock?.release()
//            }
//        }
//        stayAwake = awake
//        updateSurfaceScreenOn()
    }

    private inner class EventHandler(private val player: YouajiPlayer, looper: Looper) : Handler(looper) {

        private val MEDIA_NOP = 0 // interface test message
        private val MEDIA_PREPARED = 1
        private val MEDIA_PLAYBACK_COMPLETE = 2
        private val MEDIA_BUFFERING_UPDATE = 3
        private val MEDIA_SEEK_COMPLETE = 4
        private val MEDIA_SET_VIDEO_SIZE = 5
        private val MEDIA_STARTED = 6
        private val MEDIA_TIMED_TEXT = 99
        private val MEDIA_ERROR = 100
        private val MEDIA_INFO = 200
        private val MEDIA_CURRENT = 300

        override fun handleMessage(msg: Message) {
//            Log.i("YouajiPlayer.Message", "=== what:${msg.what} arg1:${msg.arg1} arg2:${msg.arg2} === ")
            if (player.nativeContext == 0L) {
                // cpp层的easymediaplayer实例已经不存在了
                Log.w("YouajiPlayer.TAG", "mediaplayer went away with unhandled events")
                return
            }
            when (msg.what) {
                MEDIA_PREPARED -> {
                    Log.i("YouajiPlayer.TAG", "===prepared (" + msg.arg1 + "," + msg.arg2 + ")")
                    preparedListener?.onPrepared(player)
                    return
                }

                MEDIA_PLAYBACK_COMPLETE -> {
                    Log.i("YouajiPlayer.TAG", "===play complete (" + msg.arg1 + "," + msg.arg2 + ")")
                    completionListener?.onCompletion(player)
                    stayAwake(false)
                    return
                }

                MEDIA_BUFFERING_UPDATE -> {
                    Log.i("YouajiPlayer.TAG", "===buff update (" + msg.arg1 + "," + msg.arg2 + ")")
                    loadingListener?.onLoading(player, msg.arg1)
                    return
                }

                MEDIA_STARTED -> {
                    loadingListener?.onLoading(player, 100)
                    return
                }

                MEDIA_SEEK_COMPLETE -> {
                    Log.i("YouajiPlayer.TAG", "===complete (" + msg.arg1 + "," + msg.arg2 + ")")
//                    if (mOnSeekCompleteListener != null) {
//                        mOnSeekCompleteListener.onSeekComplete(mMediaPlayer)
//                    }
                    return
                }

                MEDIA_SET_VIDEO_SIZE -> {
                    Log.i("YouajiPlayer.TAG", "===video size (" + msg.arg1 + "," + msg.arg2 + ")")
                    videoSizeChangedListener?.onVideoSizeChanged(player, msg.arg1, msg.arg2)
                    return
                }

                MEDIA_ERROR -> {

                    // For PV specific error values (msg.arg2) look in
                    // opencore/pvmi/pvmf/include/pvmf_return_codes.h
                    Log.e("YouajiPlayer.TAG", "===Error (" + msg.arg1 + "," + msg.arg2 + ")")
//                    var error_was_handled = false
                    errorListener?.onError(player, msg.arg1, msg.arg2.toString())
//                    if (mOnErrorListener != null) {
//                        error_was_handled = mOnErrorListener.onError(mMediaPlayer, msg.arg1, msg.arg2)
//                    }
//                    if (mOnCompletionListener != null && !error_was_handled) {
//                        mOnCompletionListener.onCompletion(mMediaPlayer)
//                    }
                    stayAwake(false)
                    return
                }

                MEDIA_INFO -> {
                    Log.i("YouajiPlayer.TAG", "===Info (" + msg.arg1 + "," + msg.arg2 + ")")
//                    if (msg.arg1 != MEDIA_INFO_VIDEO_TRACK_LAGGING) {
//                        Log.i("YouajiPlayer.TAG", "Info (" + msg.arg1 + "," + msg.arg2 + ")")
//                    }
//                    if (mOnInfoListener != null) {
//                        mOnInfoListener.onInfo(mMediaPlayer, msg.arg1, msg.arg2)
//                    }
                    infoListener?.onInfo(player, msg.arg1, msg.arg2)
                    // No real default action so far.
                    return
                }

                MEDIA_TIMED_TEXT -> {
                    Log.i("YouajiPlayer.TAG", "===timed (" + msg.arg1 + "," + msg.arg2 + ")")
//                    if (mOnTimedTextListener != null) {
//                        if (msg.obj == null) {
//                            mOnTimedTextListener.onTimedText(mMediaPlayer, null)
//                        } else {
//                            if (msg.obj is ByteArray) {
//                                val text = ETimedText(Rect(0, 0, 1, 1), msg.obj as String)
//                                mOnTimedTextListener.onTimedText(mMediaPlayer, text)
//                            }
//                        }
//                    }
                    return
                }

                MEDIA_NOP -> {
                    // interface test message - ignore
                    Log.i("YouajiPlayer.TAG", "nop" + msg.arg1 + "," + msg.arg2 + ")")
                }

                MEDIA_CURRENT -> {

//                    Log.i(TAG, "===current (" + msg.arg1 + "," + msg.arg2 + ")");
//                    if (mOnCurrentPositionListener != null) {
//                        mOnCurrentPositionListener.onCurrentPosition(msg.arg1.toLong(), msg.arg2.toLong())
//                    }
                    timeUpdateListener?.onTimeUpdate(player, msg.arg1.toLong(), msg.arg2.toLong())
                }

                else -> {
                    Log.e("YouajiPlayer.TAG", "Unknown message type " + msg.what)
                    return
                }
            }
        }
    }


}