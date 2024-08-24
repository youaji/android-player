package com.youaji.libs.player

import android.content.Context
import android.graphics.Bitmap
import android.net.Uri
import android.view.Surface
import java.io.FileDescriptor

interface IMediaPlayer {
    /** 视频画面承载 */
    fun setSurface(surface: Surface)

    /** */
    fun setDataSource(path: String)
    fun setDataSource(path: String, headers: Map<String, String>?)
    fun setDataSource(context: Context, uri: Uri)
    fun setDataSource(context: Context, uri: Uri, headers: Map<String, String>?)
    fun setDataSource(fileDescriptor: FileDescriptor)
    fun setDataSource(fileDescriptor: FileDescriptor, offset: Long, length: Long)

    /** 装载流媒体文件 */
    fun prepare()
    fun prepareAsync()

    /** 开始 */
    fun start()

    /** 停止 */
    fun stop()

    /** 暂停 */
    fun pause()

    /** 恢复播放 */
    fun resume()

    /** 是否循环  */
    fun setLooping(looping: Boolean)

    /** 是否循环播放 */
    fun isLooping(): Boolean

    /** 是否正在播放  */
    fun isPlaying(): Boolean

    /** 指定播放的位置 */
    fun seekTo(millisecond: Float)

    /** 得到文件的时间 */
    fun getDuration(): Long
    fun getPosition(): Long
    fun getRotate(): Int
    fun getVideoWidth(): Int
    fun getVideoHeight(): Int

    fun setMute(mute: Boolean)

    /** 设置立体音量 取值范围( 0 - 100 )； 0是静音 */
    fun setStereoVolume(leftPercent: Int, rightPercent: Int)

    /** 设置音量 取值范围( 0 - 100 )； 0是静音 */
    fun setVolume(percent: Int)

    /** 获取音量 默认100 */
    fun getVolume(): Int

    fun surfaceChange(width: Int, height: Int)
    fun setRate(rate: Float)
    fun setPitch(pitch: Float)

    /**
     * 开始录像
     * @param filePath 录像文件绝对路径，支持mp4、mov?，默认mp4
     * @return 是否成功
     */
    fun startRecord(filePath: String): Boolean

    /** 停止录像 */
    fun stopRecord(): Boolean

    /** 是否正在录像中 */
    fun isRecording(): Boolean

    /** @return 截屏 Bitmap */
    fun screenshotBitmap(): Bitmap?

    /**
     * 截屏，底层未实现。
     * 使用 [screenshotBitmap] 获取 Bitmap，自行保存
     * @param filePath  截屏文件绝对路径，支持（png、jpg），默认jpg
     * @return 是否成功
     */
    fun screenshot(filePath: String): Boolean

    /** 回收流媒体资源 */
    fun release()

    /** 装载流媒体完毕的时候回调 */
    interface OnPreparedListener {
        fun onPrepared(mediaPlayer: IMediaPlayer?)
    }

    /** 网络流媒体播放结束时回调 */
    interface OnCompletionListener {
        fun onCompletion(mediaPlayer: IMediaPlayer?)
    }

    /** 发生错误时回调 */
    interface OnErrorListener {
        fun onError(mediaPlayer: IMediaPlayer?, err: Int, message: String)
    }

    /** 加载回调 */
    interface OnLoadingListener {
        fun onLoading(mediaPlayer: IMediaPlayer?, percent: Int)
    }

    /** 视频 size 改变 */
    interface OnVideoSizeChangedListener {
        fun onVideoSizeChanged(mediaPlayer: IMediaPlayer?, width: Int, height: Int)
    }

    /** 时间更新回调 */
    interface OnTimeUpdateListener {
        fun onTimeUpdate(mediaPlayer: IMediaPlayer?, current: Long, duration: Long)
    }

    interface OnInfoListener {
        fun onInfo(mediaPlayer: IMediaPlayer?, what: Int, extra: Int)
    }

}