package com.youaji.example.player.activity

import android.Manifest
import android.graphics.Bitmap
import android.net.Uri
import android.os.Bundle
import android.os.SystemClock
import androidx.activity.result.contract.ActivityResultContracts
import androidx.core.view.isGone
import androidx.core.view.isVisible
import androidx.fragment.app.FragmentActivity
import com.youaji.example.player.databinding.ActivityPlayerBinding
import com.youaji.example.player.dialog.PlayerSetting
import com.youaji.example.player.testUrls
import com.youaji.example.player.uri2RealPath
import com.youaji.libs.player.MeasureHelper
import com.youaji.libs.player.YouajiPlayerControllerImpl
import com.youaji.libs.player.YouajiPlayerView
import com.youaji.libs.ui.basic.BasicBindingActivity
import com.youaji.libs.util.activity
import com.youaji.libs.util.design.selector
import com.youaji.libs.util.extensions.requestPermissions
import com.youaji.libs.util.externalMoviesDirPath
import com.youaji.libs.util.externalPicturesDirPath
import com.youaji.libs.util.isExistOrCreateNewFile
import com.youaji.libs.util.toast
import java.io.File
import java.io.FileOutputStream

class PlayerActivity : BasicBindingActivity<ActivityPlayerBinding>() {


    private val launcherGallery = registerForActivityResult(ActivityResultContracts.GetContent()) {
        it?.let { uri -> startPlay(uri2RealPath(uri)) }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        defaultStatusBar()

        binding.btnClose.setOnClickListener { finish() }
        binding.btnSettings.setOnClickListener {
            PlayerSetting.Builder(this)
                .setBackgroundCallback { binding.playerView.player?.changeBackground(it) }
                .setFilterCallback { binding.playerView.player?.changeFilter(it) }
                .setEffectCallback { binding.playerView.player?.changeEffect(it) }
                .setWaterMarkCallback { dataArray, dataLen, width, height, scale, location ->
                    binding.playerView.player?.setWaterMark(dataArray, dataLen, width, height, scale, location)
                }
                .build().show()
        }
        binding.btnRecord.setOnClickListener {
            if (activity is FragmentActivity) {
                (activity as FragmentActivity).requestPermissions(
                    Manifest.permission.WRITE_EXTERNAL_STORAGE,
                    Manifest.permission.READ_EXTERNAL_STORAGE,
                ) { }
            }

            if (binding.playerView.player?.isRecording() == true) {
                val stopSuccess = binding.playerView.player?.stopRecord() == true
                toast(if (stopSuccess) "停止录制" else "停止录制失败")
                binding.btnRecord.text = if (stopSuccess) "Start Record" else "Stop Record"
                binding.textRecordTime.stop()
                binding.textRecordTime.isGone = stopSuccess
            } else {

                val file = File(externalMoviesDirPath + "/" + System.currentTimeMillis() + ".mp4")
                if (file.isExistOrCreateNewFile()) {
                    val startSuccess = binding.playerView.player?.startRecord(file.absolutePath) == true
                    toast(if (startSuccess) "开始录制" else "录制失败")
                    binding.btnRecord.text = if (startSuccess) "Stop Record" else "Start Record"
                    binding.textRecordTime.isVisible = startSuccess
                    binding.textRecordTime.base = SystemClock.elapsedRealtime()
                    binding.textRecordTime.start()
                } else {
                    toast("文件创建失败！")
                }
            }
        }
        binding.btnScreenshot.setOnClickListener {
            val screenshotBitmap = binding.playerView.player?.screenshotBitmap()
            screenshotBitmap?.let {
                if (it.width * it.height <= 0) {
                    toast("宽高错误")
                    return@let
                }

                val file = File(externalPicturesDirPath + "/" + System.currentTimeMillis() + ".jpg")
                if (!file.isExistOrCreateNewFile()) {
                    toast("文件创建失败")
                    return@let
                }

                try {
                    val fos = FileOutputStream(file)
                    it.compress(Bitmap.CompressFormat.JPEG, 100, fos)
                    fos.flush()
                    fos.close()
                } catch (e: Exception) {
                    e.printStackTrace()
                    toast("流错误")
                    return@let
                }

                toast("保存至\n${file.absolutePath}")
            } ?: toast("获取失败")

//            val file = File(externalPicturesDirPath + System.currentTimeMillis() + ".jpg")
//            if (file.isExistOrCreateNewFile()) {
//                val success = binding.playerView.player?.screenshot(file.absolutePath) == true
//                toast(if (success) "保存在${file.absolutePath}" else "失败")
//            } else {
//                toast("文件创建失败！")
//            }
        }
        binding.btnUrl.setOnClickListener { chooseUrl() }
        binding.btnFile.setOnClickListener { launcherGallery.launch("video/*") }

        chooseUrl()
    }

    override fun onPause() {
        super.onPause()
        binding.playerView.pause()
    }

    override fun onResume() {
        super.onResume()
        binding.playerView.resume()
    }

    override fun onDestroy() {
        super.onDestroy()
        binding.playerView.release()
    }

    private fun chooseUrl() {
        selector(testUrls) { dialogInterface, i ->
            startPlay(testUrls[i])
            dialogInterface.dismiss()
        }
    }

    private fun startPlay(path: String) {
        binding.playerView.switchPlayerCore(YouajiPlayerView.PlayerCore.YOUAJI)
        binding.playerView.setController(YouajiPlayerControllerImpl(this), MeasureHelper.FitModel.FM_DEFAULT)
        binding.playerView.play(path)
    }

    private fun startPlay(uri: Uri) {
        binding.playerView.switchPlayerCore(YouajiPlayerView.PlayerCore.YOUAJI)
        binding.playerView.setController(YouajiPlayerControllerImpl(this), MeasureHelper.FitModel.FM_DEFAULT)
        binding.playerView.play(uri)
    }
}