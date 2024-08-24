package com.youaji.libs.player

import android.content.Context
import android.content.res.Configuration
import android.graphics.Color
import android.net.Uri
import android.util.AttributeSet
import android.view.Gravity
import android.view.TextureView
import android.view.ViewGroup
import android.widget.FrameLayout
import androidx.annotation.ColorInt
import java.lang.ref.WeakReference

class YouajiPlayerView @JvmOverloads constructor(
    private val context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0,
) : FrameLayout(context, attrs, defStyleAttr) {

    /** 播放器核心 */
    enum class PlayerCore {
        /** YouajiPlayer 内核 */
        YOUAJI,

        /** 系统 MediaPlayer 内核 */
        SYSTEM
    }

    var player: BasicMediaPlayer? = null
        private set
    var playerCore = PlayerCore.YOUAJI
        private set
    var textureView: TextureView? = null
        private set
    var container: FrameLayout? = null
        private set
    private var playerController: YouajiPlayerController? = null
    private var measureHelper: MeasureHelper? = null

    /** 普通模式 */
    private val modeNormal = 0

    /** 全屏模式 */
    private val modeFull = 1
    private var currentMode = modeNormal

    init {
        measureHelper = object : MeasureHelper(this) {
            override val isFullScreen: Boolean
                get() = currentMode == modeFull
        }
        initContainer()
        keepScreenOn = true //设置屏幕保持常亮

    }

    /** 切换播放器内核 */
    fun switchPlayerCore(playerCore: PlayerCore) {
        this.playerCore = playerCore
    }

    /**
     * 初始化播放器
     */
    private fun initPlayer() {
        if (textureView == null) {
            textureView = ScaleTextureView(context)
        }
        if (playerCore == PlayerCore.SYSTEM) {
            // 系统 MediaPlayer 内核
//            mPlayer = SystemMediaPlayerImpl()
        } else {
            // YouajiPlayer 内核
            player = YouajiPlayerImpl()
        }
        player?.setTextureView(textureView!!)
        player?.setOnVideoSizeChangedListener(VideoSizeChangedListener(this))
    }

    //更新view尺寸
    private fun updateVideoLayoutParams(width: Int, height: Int) {
        post {
            measureHelper?.videoSizeInfo = MeasureHelper.VideoSizeInfo(width, height)
            measureHelper?.setVideoLayoutParams(textureView, container)
        }
    }

    /**
     * 视频size改变 监听
     */
    class VideoSizeChangedListener internal constructor(playerView: YouajiPlayerView) : IMediaPlayer.OnVideoSizeChangedListener {
        private val playerViewWeakReference: WeakReference<YouajiPlayerView>

        init {
            playerViewWeakReference = WeakReference(playerView)
        }

        override fun onVideoSizeChanged(mediaPlayer: IMediaPlayer?, width: Int, height: Int) {
            playerViewWeakReference.get()?.updateVideoLayoutParams(width, height)
        }
    }


    private fun initContainer() {
        this.container = FrameLayout(context)
        this.container?.setBackgroundColor(Color.BLACK)
        this.addView(container, LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT))
    }

    /**
     * 设置播放器背景颜色
     *
     * @param color
     */
    fun setPlayerBackgroundColor(@ColorInt color: Int) {
        this.container?.setBackgroundColor(color)
    }

    /**
     * 设置设置控制层容器
     *
     * @param playerController 控制层容器
     * @param fitModel         设置视频尺寸适配模式
     */
    fun setController(playerController: YouajiPlayerController, fitModel: MeasureHelper.FitModel) {
        initPlayer()
        setFitModel(fitModel)
        this.container?.removeView(this.playerController)
        this.playerController = playerController
        this.playerController?.setPlayerView(this)
        this.container?.addView(playerController, LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT))
        addTextureView()
    }

    private fun addTextureView() {
        this.container?.removeView(textureView)
        this.container?.addView(textureView, 0, LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT, Gravity.CENTER))
    }

    /**
     * 设置 TextureView 是否启用 缩放旋转手势
     * @param enabled true:启用（默认）；false:禁用
     */
    fun setTextureViewEnabledTouch(enabled: Boolean) {
        if (textureView != null && textureView is ScaleTextureView) {
            (textureView as ScaleTextureView).setEnabledTouch(enabled)
        }
    }

    /**
     * 设置播放完成回调
     * @param listener -
     */
    fun setOnCompleteListener(listener: IMediaPlayer.OnCompletionListener?) {
        this.player?.setOnCompleteListener(listener)
    }

    /**
     * 设置适配模式
     *
     * @param fitModel -
     */
    fun setFitModel(fitModel: MeasureHelper.FitModel) {
        this.measureHelper?.fitModel = fitModel
        //设置默认的 宽高
        this.measureHelper?.setDefaultVideoLayoutParams()
    }

    /**
     * 播放
     */
    fun play(path: String) {
        if (!Helper.isFastClick) {
            this.player?.play(path)
            this.playerController?.onResume()
        }
    }

    fun play(uri: Uri) {
        if (!Helper.isFastClick) {
            this.player?.play(uri)
            this.playerController?.onResume()
        }
    }

    /** 重新播放 */
    fun repeatPlay() {
        this.player?.repeatPlay()
    }

    /** 暂停 */
    fun pause() {
        this.player?.pause()
        this.playerController?.onPause()
    }

    /** 恢复播放 */
    fun resume() {
        this.player?.resume()
        this.playerController?.onResume()
    }

    /** 是否在播放 */
    fun isPlaying(): Boolean =
        this.player?.isPlaying() == true

    /**
     * 是否循环
     *
     * @return -
     */
    fun isLooping(): Boolean =
        this.player?.isLooping() == true

//    /** 停止 */
//    fun stop() {
//        this.player?.stop()
//    }

    fun setMute(mute: Boolean) {
        this.player?.setMute(mute)
    }

    /**
     * 设置音量 (需要在play方法之前调用)
     * @param percent 取值范围( 0 - 100 )； 0是静音
     */
    fun setVolume(percent: Int) {
        this.player?.setVolume(percent)
    }

    /** 获取音量 */
    fun getVolume(): Int =
        if (player?.getVolume() != -1) player?.getVolume() ?: 100 else 100

    /** 销毁 */
    fun release() {
        this.player?.release()
        this.player = null
        this.keepScreenOn = false // 设置屏幕保持常亮
    }

    /** 当前是否是全屏 */
    fun isFullScreenModel(): Boolean =
        currentMode == modeFull


    /** 切换全屏或关闭全屏
     * @return true已经进入到全屏
     */
    fun switchScreen(): Boolean =
        if (isFullScreenModel())// 是全屏 则退出全屏
            exitFullScreen() // 退出全屏
        else
            enterFullScreen() //进入全屏

    /**  进入全屏 */
    fun enterFullScreen(): Boolean {
        if (currentMode == modeFull) return false
        val decorView = Helper.setFullScreen(context, true) ?: return false
        this.removeView(container)
        decorView.addView(container, LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT))
        currentMode = modeFull
        return true
    }

    /** 退出全屏 */
    fun exitFullScreen(): Boolean {
        if (currentMode == modeFull) {
            val decorView = Helper.setFullScreen(context, false) ?: return false
            decorView.removeView(container)
            this.addView(container, LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT))
            currentMode = modeNormal
        }
        return false
    }

    // 屏幕旋转后改变
    override fun onConfigurationChanged(newConfig: Configuration?) {
        super.onConfigurationChanged(newConfig)
        // 屏幕旋转后更新layout尺寸
        measureHelper?.setVideoLayoutParams(textureView, container)
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec)
        measureHelper?.doMeasure(measuredWidth, measuredHeight)?.let { size ->
            if (size.size >= 2) setMeasuredDimension(size[0], size[1])
        }
    }
}