package com.youaji.libs.player

import android.net.Uri
import android.view.TextureView

abstract class BasicMediaPlayer : IMediaPlayer {

    protected var preparedListener: IMediaPlayer.OnPreparedListener? = null
    protected var videoSizeChangedListener: IMediaPlayer.OnVideoSizeChangedListener? = null
    protected var loadingListener: IMediaPlayer.OnLoadingListener? = null
    protected var timeUpdateListener: IMediaPlayer.OnTimeUpdateListener? = null
    protected var errorListener: IMediaPlayer.OnErrorListener? = null
    protected var completionListener: IMediaPlayer.OnCompletionListener? = null
    protected var infoListener: IMediaPlayer.OnInfoListener? = null

    /** 设置 TextureView */
    abstract fun setTextureView(textureView: TextureView)
    abstract fun play(path: String)
    abstract fun play(uri: Uri)

    /** 重复播放 子类快捷实现  */
    abstract fun repeatPlay()

    abstract fun changeBackground(background: Background)
    abstract fun changeEffect(effect: Effect)
    abstract fun changeFilter(filter: Filter)
    abstract fun setWaterMark(dataArray: ByteArray, dataLen: Int, width: Int, height: Int, scale: Float, location: WaterMarkLocation)

    fun setOnPreparedListener(listener: IMediaPlayer.OnPreparedListener?) {
        preparedListener = listener
    }

    fun setOnVideoSizeChangedListener(listener: IMediaPlayer.OnVideoSizeChangedListener?) {
        videoSizeChangedListener = listener
    }

    fun setOnLoadingListener(listener: IMediaPlayer.OnLoadingListener?) {
        loadingListener = listener
    }

    fun setOnTimeUpdateListener(listener: IMediaPlayer.OnTimeUpdateListener?) {
        timeUpdateListener = listener
    }

    fun setOnErrorListener(listener: IMediaPlayer.OnErrorListener?) {
        errorListener = listener
    }

    fun setOnCompleteListener(listener: IMediaPlayer.OnCompletionListener?) {
        completionListener = listener
    }

    fun setOnInfoListener(listener: IMediaPlayer.OnInfoListener?) {
        infoListener = listener
    }
}