package com.youaji.example.player.activity

import android.annotation.SuppressLint
import android.net.Uri
import android.os.Bundle
import android.text.SpannableStringBuilder
import android.view.SurfaceHolder
import android.view.SurfaceHolder.Callback
import androidx.activity.result.contract.ActivityResultContracts
import com.youaji.example.player.databinding.ActivitySurfaceBinding
import com.youaji.example.player.testUrls
import com.youaji.example.player.uri2RealPath
import com.youaji.libs.player.IMediaPlayer
import com.youaji.libs.player.YouajiPlayer
import com.youaji.libs.ui.basic.BasicBindingActivity
import com.youaji.libs.util.design.selector


class SurfaceActivity : BasicBindingActivity<ActivitySurfaceBinding>() {

    private lateinit var player: YouajiPlayer
    private val launcherGallery = registerForActivityResult(ActivityResultContracts.GetContent()) {
        it?.let { uri -> startPlay(uri2RealPath(uri)) }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setToolbar("Surface")
        initView()
        initPlayer()
        chooseUri()
    }

    override fun onPause() {
        super.onPause()
        player.pause()
    }

    override fun onResume() {
        super.onResume()
        player.resume()
    }

    override fun onDestroy() {
        super.onDestroy()
        player.stop()
        player.release()
    }

    private fun initView() {
        binding.btnStart.setOnClickListener { player.start() }
        binding.btnPause.setOnClickListener { player.pause() }
        binding.btnStop.setOnClickListener { player.stop() }
        binding.btnResume.setOnClickListener { player.resume() }
        binding.btnFile.setOnClickListener { launcherGallery.launch("video/*") }
        binding.btnUrl.setOnClickListener { chooseUri() }
        binding.surfaceView.holder.addCallback(object : Callback {
            override fun surfaceCreated(holder: SurfaceHolder) {
                player.setSurface(holder.surface)
            }

            override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
                player.surfaceChange(width, height)
            }

            override fun surfaceDestroyed(holder: SurfaceHolder) {}

        })
    }

    private fun initPlayer() {
        player = YouajiPlayer()
        player.setOnPreparedListener(object : IMediaPlayer.OnPreparedListener {
            override fun onPrepared(mediaPlayer: IMediaPlayer?) {
                player.start()
                setFixedInfo()
                changeInfo = ChangeInfo()
                refreshInfo()
            }
        })
        player.setOnInfoListener(object : IMediaPlayer.OnInfoListener {
            override fun onInfo(mediaPlayer: IMediaPlayer?, what: Int, extra: Int) {
                changeInfo.infoWhat = what
                changeInfo.infoExtra = extra
                refreshInfo()
            }
        })
        player.setOnErrorListener(object : IMediaPlayer.OnErrorListener {
            override fun onError(mediaPlayer: IMediaPlayer?, err: Int, message: String) {
                changeInfo.error = err
                changeInfo.message = message
                refreshInfo()
            }
        })
        player.setOnCompleteListener(object : IMediaPlayer.OnCompletionListener {
            override fun onCompletion(mediaPlayer: IMediaPlayer?) {
                changeInfo.isCompletion = true
                refreshInfo()
            }
        })
        player.setOnTimeUpdateListener(object : IMediaPlayer.OnTimeUpdateListener {
            override fun onTimeUpdate(mediaPlayer: IMediaPlayer?, current: Long, duration: Long) {
                changeInfo.current = current
                changeInfo.duration = duration
                refreshInfo()
            }
        })
        player.setOnLoadingListener(object : IMediaPlayer.OnLoadingListener {
            override fun onLoading(mediaPlayer: IMediaPlayer?, percent: Int) {
                changeInfo.percent = percent
                refreshInfo()
            }
        })
        player.setOnVideoSizeChangedListener(object : IMediaPlayer.OnVideoSizeChangedListener {
            override fun onVideoSizeChanged(mediaPlayer: IMediaPlayer?, width: Int, height: Int) {
                changeInfo.videoWidth = width
                changeInfo.videoHeight = height
                refreshInfo()
            }
        })
    }

    @SuppressLint("SetTextI18n")
    private fun setFixedInfo() {
        binding.textFixedInfo.text = "" +
                "videoWidth:${player.getVideoWidth()} videoHeight:${player.getVideoHeight()}\n" +
                "duration:${player.getDuration()} rotate:${player.getRotate()} isLooping:${player.isLooping()}"
    }

    private var changeInfo = ChangeInfo()

    data class ChangeInfo(
        var current: Long? = null,
        var duration: Long? = null,
        var error: Int? = null,
        var message: String? = null,
        var infoWhat: Int? = null,
        var infoExtra: Int? = null,
        var isCompletion: Boolean? = null,
        var percent: Int? = null,
        var videoWidth: Int? = null,
        var videoHeight: Int? = null,
    )

    private fun refreshInfo() {
        val spannableString = SpannableStringBuilder("")
        if (changeInfo.current != null && changeInfo.duration != null) {
            spannableString.append("current:${changeInfo.current} duration:${changeInfo.duration}\n")
        }
        if (changeInfo.error != null && changeInfo.message != null) {
            spannableString.append("error:${changeInfo.error} message:${changeInfo.message}\n")
        }
        if (changeInfo.infoWhat != null && changeInfo.infoExtra != null) {
            spannableString.append("infoWhat:${changeInfo.infoWhat} message:${changeInfo.infoExtra}\n")
        }
        if (changeInfo.isCompletion != null) {
            spannableString.append("isCompletion:${changeInfo.isCompletion}\n")
        }
        if (changeInfo.percent != null) {
            spannableString.append("percent:${changeInfo.percent}\n")
        }
        if (changeInfo.videoWidth != null && changeInfo.videoHeight != null) {
            spannableString.append("videoWidth:${changeInfo.videoWidth} message:${changeInfo.videoHeight}\n")
        }
        spannableString.append("isPlaying:${player.isPlaying()} volume:${player.getVolume()}")
        binding.textChangeInfo.text = spannableString.toString()
    }

    private fun chooseUri() {
        selector(testUrls) { dialogInterface, i ->
            startPlay(path = testUrls[i])
            dialogInterface.dismiss()
        }
    }

    private fun startPlay(path: String? = null, uri: Uri? = null) {
        player.reset()
        initPlayer()
        if (path != null) player.setDataSource(path)
        else if (uri != null) player.setDataSource(this, uri)
        else return
        player.setSurface(binding.surfaceView.holder.surface)
        player.prepareAsync()
    }
}