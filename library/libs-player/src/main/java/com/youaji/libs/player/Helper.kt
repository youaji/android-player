package com.youaji.libs.player

import android.annotation.SuppressLint
import android.app.Activity
import android.content.Context
import android.content.ContextWrapper
import android.content.pm.ActivityInfo
import android.os.Build
import android.util.DisplayMetrics
import android.view.View
import android.view.ViewGroup
import android.view.WindowManager

object Helper {
    fun getScreenWidth(context: Context): Int {
        val displayMetrics = context.resources.displayMetrics
        return displayMetrics.widthPixels
    }

    fun getScreenHeight(context: Context): Int {
        val displayMetrics = context.resources.displayMetrics
        return displayMetrics.heightPixels
    }

    /** 获取全屏的 高 去除顶部状态栏和底部导航栏 */
    fun getFullScreenHeight(context: Context): Int {
        val outMetrics = DisplayMetrics()
        scanForActivity(context)?.windowManager?.defaultDisplay?.getRealMetrics(outMetrics)
        return outMetrics.heightPixels
    }

    /** 通过 context 找到 activity */
    fun scanForActivity(context: Context): Activity? {
        return when (context) {
            is Activity -> context
            is ContextWrapper -> scanForActivity(context.baseContext)
            else -> null
        }
    }

    /** 获取DecorView */
    fun getDecorView(context: Context): ViewGroup? {
        val activity = scanForActivity(context) ?: return null
        return activity.window.decorView as ViewGroup
    }

    /** 获取DecorView */
    fun getDecorView(activity: Activity?): ViewGroup? {
        if (activity == null) return null
        return activity.window?.decorView as ViewGroup
    }

    @SuppressLint("ObsoleteSdkInt")
    fun showSysBar(activity: Activity?, decorView: ViewGroup) {
        var uiOptions = decorView.systemUiVisibility
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
            uiOptions = uiOptions and View.SYSTEM_UI_FLAG_HIDE_NAVIGATION.inv()
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            uiOptions = uiOptions and View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY.inv()
        }
        decorView.systemUiVisibility = uiOptions
        activity?.window?.clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN)
    }

    @SuppressLint("ObsoleteSdkInt")
    fun hideSysBar(activity: Activity?, decorView: ViewGroup) {
        var uiOptions = decorView.systemUiVisibility
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
            uiOptions = uiOptions or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            uiOptions = uiOptions or View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
        }
        decorView.systemUiVisibility = uiOptions
        activity?.window?.setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN)
    }

    /** 设置全屏 */
    @SuppressLint("SourceLockedOrientationActivity")
    @JvmStatic
    fun setFullScreen(context: Context, isFullScreen: Boolean): ViewGroup? {
        val activity = scanForActivity(context)
        val decorView = getDecorView(activity) ?: return null
        if (isFullScreen) { //全屏
            // 隐藏ActionBar、状态栏，并横屏
            hideSysBar(activity, decorView)
            activity?.requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE
        } else { //非全屏
            // 展示ActionBar、状态栏，并竖屏
            showSysBar(activity, decorView)
            activity?.requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT
        }
        return decorView
    }

    /**
     * 时间显示(只显示到秒)
     * @param millisSeconds
     * @return
     */
    fun generateStandardTime(millisSeconds: Long): String {
        val totalSeconds = millisSeconds.toInt() / 1000
        val seconds = totalSeconds % 60
        val minutes = totalSeconds / 60 % 60
        val hours = minutes / 3600 % 60
        return if (hours > 0) {
            String.format("%02d:%02d:%02d", hours, minutes, seconds)
        } else {
            String.format("%02d:%02d", minutes, seconds)
        }
    }

    fun secondToDateFormat(second: Int, totalSecond: Int): String {
        val hours = (second / (60 * 60)).toLong()
        val minutes = (second % (60 * 60) / 60).toLong()
        val seconds = (second % 60).toLong()
        var sh = "00"
        if (hours > 0) {
            sh = if (hours < 10) "0$hours" else hours.toString() + ""
        }
        var sm = "00"
        if (minutes > 0) {
            sm = if (minutes < 10) "0$minutes" else minutes.toString() + ""
        }
        var ss = "00"
        if (seconds > 0) {
            ss = if (seconds < 10) "0$seconds" else seconds.toString() + ""
        }
        return if (totalSecond >= 3600) "$sh:$sm:$ss" else "$sm:$ss"
    }

    private var lastClickTime = 0L

    @JvmStatic
    @get:Synchronized
    val isFastClick: Boolean
        /**
         * 防止快速点击
         * @return true:是快速点击； false:否
         */
        get() {
            val time = System.currentTimeMillis()
            if (time - lastClickTime < 1000) {
                return true
            }
            lastClickTime = time
            return false
        }
}
