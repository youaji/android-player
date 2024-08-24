package com.youaji.libs.player

import android.graphics.Bitmap
import android.graphics.SurfaceTexture
import android.net.Uri
import android.view.Surface
import android.view.TextureView
import java.lang.ref.WeakReference

class YouajiPlayerImpl : YouajiPlayer(), TextureView.SurfaceTextureListener {

    private var textureViewWeakReference: WeakReference<TextureView>? = null
    private var surfaceTexture: SurfaceTexture? = null
    private val textureView: TextureView?
        get() = textureViewWeakReference?.get()

    override fun setTextureView(textureView: TextureView) {
        textureViewWeakReference = WeakReference(textureView)
        textureView.surfaceTextureListener = this
    }

    override fun play(path: String) {
        if (path.isNotEmpty()) {
//            stop()
            reset()
            setDataSource(path)
//            setLooping(isLooping)
            setOnPreparedListener(object : IMediaPlayer.OnPreparedListener {
                override fun onPrepared(mediaPlayer: IMediaPlayer?) {
                    start()
                }
            })
            prepareAsync()
        }
    }

    override fun play(uri: Uri) {
        textureView?.context?.let {
            stop()
            setDataSource(it, uri)
//            setLooping(isLooping)
            setOnPreparedListener(object : IMediaPlayer.OnPreparedListener {
                override fun onPrepared(mediaPlayer: IMediaPlayer?) {
                    start()
                }
            })
            prepareAsync()
        }
    }

    override fun screenshotBitmap(): Bitmap? {
        return textureView?.bitmap
    }

    override fun release() {
        stop()
        super.release()
        surfaceTexture?.release()
        surfaceTexture = null
    }

    override fun onSurfaceTextureAvailable(surface: SurfaceTexture, width: Int, height: Int) {
        if (textureView == null) {
            return
        }
        // 解决 暂停状态下，旋转屏幕出现黑屏情况，
        // 因旋转屏幕后会重新生成一个surfaceTexture，
        // 这里把mSurfaceTexture保存起来，旋转后直接恢复第一个surfaceTexture  mTextureView.setSurfaceTexture(mSurfaceTexture);
        // 解决 暂停状态下，旋转屏幕出现黑屏情况，
        // 因旋转屏幕后会重新生成一个surfaceTexture，
        // 这里把mSurfaceTexture保存起来，旋转后直接恢复第一个surfaceTexture  mTextureView.setSurfaceTexture(mSurfaceTexture);
        if (surfaceTexture == null) {
            surfaceTexture = surface
            setSurface(Surface(surfaceTexture))
        } else {
            textureView?.setSurfaceTexture(surfaceTexture!!)
        }
    }

    override fun onSurfaceTextureSizeChanged(surface: SurfaceTexture, width: Int, height: Int) {
        surfaceChange(width, height)
    }

    override fun onSurfaceTextureDestroyed(surface: SurfaceTexture): Boolean = false

    override fun onSurfaceTextureUpdated(surface: SurfaceTexture) {}
}