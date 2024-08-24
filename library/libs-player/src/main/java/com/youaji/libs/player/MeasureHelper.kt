package com.youaji.libs.player

import android.view.Gravity
import android.view.TextureView
import android.view.View
import android.widget.FrameLayout
import java.lang.ref.WeakReference

open class MeasureHelper(view: View) {
    private val viewWeakReference: WeakReference<View>

    init {
        viewWeakReference = WeakReference(view)
    }

    open val isFullScreen: Boolean = false

    private val measuredWidth = 0
    private var measuredHeight = 0

    /** 适配模式 */
    var fitModel = FitModel.FM_DEFAULT

    /** 适配模式 */
    enum class FitModel {
        /** 默认, 宽铺满，横屏的视频高度自适应，如果是竖屏的视频，高度等于屏幕宽 */
        FM_DEFAULT,

        /** 全屏，宽铺满 高自适应 */
        FM_FULL_SCREEN_WIDTH,

        /** 全屏，高铺满 宽自适应 */
        FM_FULL_SCREEN_HEIGHT,

        /** 宽高比：16：9 */
        FM_WH_16X9
    }

    /** 视频信息 */
    var videoSizeInfo: VideoSizeInfo? = null

    /**
     * 视频信息
     * @param width     宽
     * @param height    高
     */
    class VideoSizeInfo(val width: Int, val height: Int)

    val view: View?
        get() = viewWeakReference.get()

    /** 设置默认的播放器容器宽高 */
    fun setDefaultVideoLayoutParams() {
        val view = view
        val playerView: YouajiPlayerView?
        if (view is YouajiPlayerView) {
            playerView = view
            val width = Helper.getScreenWidth(view.getContext()) //宽
            val height = width * 9 / 16 // 高
            videoSizeInfo = VideoSizeInfo(width, height)
            setVideoLayoutParams(playerView.textureView, playerView.container)
        }
    }

    fun setVideoLayoutParams(textureView: TextureView?, container: FrameLayout?) {
        if (textureView == null || container == null || videoSizeInfo == null) {
            return
        }
        val videoWidth = videoSizeInfo?.width ?: 0
        val videoHeight = videoSizeInfo?.height ?: 0
        view?.let { v ->
            //原始视频宽高比
            val videoAspect = videoWidth.toFloat() / videoHeight
            var viewWidth: Int = Helper.getScreenWidth(v.context)
            val viewHeight: Int
            if (isFullScreen) { //全屏
                //高度铺满
                viewHeight = Helper.getScreenHeight(v.context)
                //宽度按比例
                viewWidth = (viewHeight * videoAspect).toInt()
            } else { //非全屏
                if (fitModel == FitModel.FM_FULL_SCREEN_WIDTH) {
                    //宽铺满，高度按比例
                    viewHeight = (viewWidth / videoAspect).toInt()
                } else if (fitModel == FitModel.FM_FULL_SCREEN_HEIGHT) {
                    //高度铺满
                    viewHeight = Helper.getFullScreenHeight(v.context)
                    //宽自适应
                    viewWidth = (viewHeight * videoAspect).toInt()
                } else if (fitModel == FitModel.FM_WH_16X9) {
                    viewHeight = viewWidth * 9 / 16
                    viewWidth = (viewHeight * videoAspect).toInt()
                } else {
                    if (videoWidth > videoHeight) { //横屏视频
                        //宽铺满，高度按比例
                        viewHeight = (viewWidth / videoAspect).toInt()
                    } else if (videoWidth < videoHeight) { //竖屏视频
                        //高铺满 宽自适应
                        viewHeight = viewWidth
                        viewWidth = (viewHeight * videoAspect).toInt()
                    } else { //正方形视频
                        viewHeight = viewWidth
                    }
                }
            }
            val textureParams = FrameLayout.LayoutParams(viewWidth, viewHeight)
            textureParams.gravity = Gravity.CENTER
            textureView.setLayoutParams(textureParams)

            // 容器的宽固定铺满状态，高度跟随 playerView 的高
            val containerParams = FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT, viewHeight)
//            val containerParams = FrameLayout.LayoutParams(Helper.getScreenWidth(v.context), viewHeight)
            container.setLayoutParams(containerParams)
            measuredHeight = viewHeight
            v.requestLayout()
        }
    }

    /** 开始适配 */
    fun doMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int): IntArray {
        val viewWidth: Int
        val viewHeight: Int
        if (fitModel == FitModel.FM_DEFAULT || fitModel == FitModel.FM_FULL_SCREEN_HEIGHT) {
            viewWidth = widthMeasureSpec
            viewHeight = measuredHeight
        } else {
            viewWidth = widthMeasureSpec
            viewHeight = heightMeasureSpec
        }
        val size = IntArray(2)
        size[0] = viewWidth
        size[1] = viewHeight
        return size
    }
}
