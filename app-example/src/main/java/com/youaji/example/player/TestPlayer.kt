package com.youaji.example.player

import android.net.Uri
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.TextureView
import com.youaji.libs.player.IMediaPlayer
import com.youaji.libs.player.YouajiPlayer

class TestPlayer : YouajiPlayer() {
    override fun setTextureView(textureView: TextureView) {

    }

    override fun play(path: String) {
    }

    override fun play(uri: Uri) {
    }

    fun init(path: String, surfaceView: SurfaceView) {
        setDataSource(path)
        setOnPreparedListener(object : IMediaPlayer.OnPreparedListener {
            override fun onPrepared(mediaPlayer: IMediaPlayer?) {
                start()
            }
        })
        prepare()

        surfaceView.holder.addCallback(object : SurfaceHolder.Callback {
            override fun surfaceCreated(holder: SurfaceHolder) {
                setSurface(holder.surface)
            }

            override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
            }

            override fun surfaceDestroyed(holder: SurfaceHolder) {
            }
        })
    }

}