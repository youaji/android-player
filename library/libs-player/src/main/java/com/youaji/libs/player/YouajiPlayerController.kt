package com.youaji.libs.player

import android.content.Context
import android.view.LayoutInflater
import android.widget.FrameLayout
import androidx.annotation.LayoutRes

abstract class YouajiPlayerController(context: Context) : FrameLayout(context) {

    @JvmField
    protected var playerView: YouajiPlayerView? = null
    protected var player: BasicMediaPlayer? = null
        private set

    init {
        LayoutInflater.from(getContext()).inflate(this.getLayoutId(), this, true)
        this.initView()
    }

    fun setPlayerView(playerView: YouajiPlayerView) {
        this.playerView = playerView
        this.player = this.playerView?.player
        initListener()
    }

    /** 提供 layout */
    @LayoutRes
    protected abstract fun getLayoutId(): Int

    /** 初始化view */
    protected abstract fun initView()

    /** 设置播放器动作Listener */
    protected abstract fun initListener()

    /** 播放器触发了 Pause */
    abstract fun onPause()

    /** 播放器触发了 Resume */
    abstract fun onResume()
}