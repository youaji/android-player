package com.youaji.libs.player

import android.annotation.SuppressLint
import android.content.Context
import android.view.View
import android.widget.ImageView
import android.widget.ProgressBar
import android.widget.SeekBar
import android.widget.SeekBar.OnSeekBarChangeListener
import android.widget.TextView
import android.widget.Toast
import androidx.core.view.isGone
import androidx.core.view.isVisible
import java.lang.ref.WeakReference

class YouajiPlayerControllerImpl(context: Context) : YouajiPlayerController(context) {

    private var timeView: TextView? = null
    private var progressView: SeekBar? = null
    private var progressBar: ProgressBar? = null
    private var bottomPanel: View? = null
    private var playBtn: ImageView? = null
    private var repeatPlay: View? = null
    private var muteImage: ImageView? = null //静音图标

    private var isSeeking = false
    var position = 0L

    override fun getLayoutId(): Int = R.layout.youaji_player_controller

    override fun initView() {
        bottomPanel = findViewById(R.id.bottomPanel)
        progressView = findViewById(R.id.progress_view)
        timeView = findViewById(R.id.time_view)
        progressBar = findViewById(R.id.progressBar)
        playBtn = findViewById(R.id.iv_play)
        repeatPlay = findViewById(R.id.repeatPlay)
        repeatPlay?.setOnClickListener { //隐藏重播按钮
            playerView?.repeatPlay()
            repeatPlay?.visibility = GONE
        }
        val iconFullScreen = findViewById<ImageView>(R.id.iv_fullscreen)
        iconFullScreen.setOnClickListener {
            //屏幕旋转 全屏
            playerView?.switchScreen()
            iconFullScreen.setImageResource(
                if (playerView?.isFullScreenModel() == true) R.drawable.youaji_player_fullscreen_exit_24_white
                else R.drawable.youaji_player_fullscreen_enter_24_white
            )
        }
        muteImage = findViewById(R.id.iv_mute)
        muteImage?.setOnClickListener { //静音
            switchMute()
        }
        playBtn?.setOnClickListener {
            if (playerView?.isPlaying() == true) { //暂停播放
                playerView?.pause()
            } else { //恢复播放
                playerView?.resume()
            }
        }
    }

    fun switchMute() {
        val setVolumeBefore = playerView?.getVolume()
        if ((playerView?.getVolume() ?: 0) <= 0) {
            //当前是静音，设置为非静音
            playerView?.setVolume(100)
            muteImage?.setImageResource(R.drawable.youaji_player_volume_on_24_black)
        } else {
            //当前不是静音，设置为静音
            playerView?.setVolume(0)
            muteImage?.setImageResource(R.drawable.youaji_player_volume_off_24_black)
        }
        val setVolumeAfter = playerView?.getVolume()
    }

    override fun initListener() {
        val playerListener = PlayerListener(this)
        player?.setOnLoadingListener(playerListener)
        player?.setOnTimeUpdateListener(playerListener)
        player?.setOnErrorListener(playerListener)
        player?.setOnCompleteListener(playerListener)
        progressView?.setOnSeekBarChangeListener(object : OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {
                player?.let { p ->
                    position = progress * p.getDuration() / 100L
                    if (isSeeking) {
                        if (playerView?.playerCore == YouajiPlayerView.PlayerCore.YOUAJI) {
                            // YouajiPlayer 内核 返回的时间单位是秒
                            onTimeUpdate(null, position, p.getDuration())
                        } else if (playerView?.playerCore === YouajiPlayerView.PlayerCore.SYSTEM) {
                            // 系统 MediaPlayer 内核 返回的时间单位是毫秒秒
                            onTimeUpdate(null, position, p.getDuration())
                        }
                        p.seekTo(position.toFloat())
                    }
                }
            }

            override fun onStartTrackingTouch(seekBar: SeekBar) {
                isSeeking = true
                player?.pause() //拖动进度条时 暂停播放
            }

            override fun onStopTrackingTouch(seekBar: SeekBar) {
                player?.resume() //拖动进度条结束后 恢复播放
                player?.seekTo(position.toFloat())
                isSeeking = false
            }
        })
    }

    /** 播放完成 */
    fun onCompletion(mediaPlayer: IMediaPlayer?) {
        // 不是循环模式 显示重新播放页面
        post { repeatPlay?.isVisible = playerView?.isLooping() == false }
    }

    /** 播放出错 */
    fun onError(mediaPlayer: IMediaPlayer?, code: Int, message: String) {
        post { Toast.makeText(context, "出错了：code=$code, msg=$message", Toast.LENGTH_SHORT).show() }
    }

    /** 时间更新 */
    @SuppressLint("SetTextI18n")
    fun onTimeUpdate(mediaPlayer: IMediaPlayer?, currentTime: Long, totalTime: Long) {
        post(Runnable {
            // 总时长为0 (直播视频)，则隐藏时间进度条
            bottomPanel?.isGone = totalTime <= 0
            if (bottomPanel?.isGone == true) return@Runnable
            val current = Helper.generateStandardTime(currentTime.coerceAtLeast(0))
            val duration = Helper.generateStandardTime(totalTime.coerceAtLeast(0))
            timeView?.text = "$current / $duration"
            if (!isSeeking && totalTime > 0) {
                progressView?.progress = (currentTime * 100 / totalTime).toInt()
            }
        })
    }

    /**
     * 加载状态
     */
    fun onLoading(mediaPlayer: IMediaPlayer?, isLoading: Boolean) {
        post { progressBar?.isVisible = isLoading }
    }

    /**
     * 播放器监听
     */
    class PlayerListener internal constructor(playerControllerImpl: YouajiPlayerControllerImpl) :
        IMediaPlayer.OnCompletionListener,
        IMediaPlayer.OnErrorListener,
        IMediaPlayer.OnLoadingListener,
        IMediaPlayer.OnTimeUpdateListener {

        private val controllerWeakReference: WeakReference<YouajiPlayerControllerImpl>

        init {
            controllerWeakReference = WeakReference(playerControllerImpl)
        }

        override fun onCompletion(mediaPlayer: IMediaPlayer?) {
            controllerWeakReference.get()?.onCompletion(mediaPlayer)
        }

        override fun onError(mediaPlayer: IMediaPlayer?, err: Int, message: String) {
            controllerWeakReference.get()?.onError(mediaPlayer, err, message)
        }

        override fun onLoading(mediaPlayer: IMediaPlayer?, percent: Int) {
            controllerWeakReference.get()?.onLoading(mediaPlayer, percent < 100)
        }

        override fun onTimeUpdate(mediaPlayer: IMediaPlayer?, current: Long, duration: Long) {
            // 这个时间有问题
            controllerWeakReference.get()?.onTimeUpdate(mediaPlayer, current, duration)
        }
    }

    override fun onPause() {
        playBtn?.setImageResource(R.drawable.youaji_player_play_24_black)
        playBtn?.animate()?.alpha(1f)?.start() //显示 播放按钮
    }

    override fun onResume() {
        playBtn?.setImageResource(R.drawable.youaji_player_pause_24_black)
        playBtn?.animate()?.alpha(1f)?.start() //隐藏 播放按钮
        //设置静音图标
        playerView?.let { pv ->
            muteImage?.setImageResource(
                if (pv.getVolume() == 0) R.drawable.youaji_player_volume_off_24_black
                else R.drawable.youaji_player_volume_on_24_black
            )
        }
    }
}