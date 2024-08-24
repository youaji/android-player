package com.youaji.example.player

import android.content.Context
import android.database.Cursor
import android.net.Uri
import android.provider.MediaStore

val testUrls by lazy {
    listOf(
        "http://220.161.87.62:8800/hls/0/index.m3u8",
        "http://devimages.apple.com/iphone/samples/bipbop/bipbopall.m3u8",
        "http://vjs.zencdn.net/v/oceans.mp4",
        "https://media.w3.org/2010/05/sintel/trailer.mp4",
        "http://clips.vorwaerts-gmbh.de/big_buck_bunny.mp4",
        "https://static.smartisanos.cn/common/video/t1-ui.mp4",
        "https://static.smartisanos.cn/common/video/video-jgpro.mp4",
        "https://qywapp.oss-cn-beijing.aliyuncs.com/2022/7/18/oss06ac675d-c493-2121-cfba-e5d627210036.mp4",
        "rtsp://192.168.230.1:8554/stream/av1_0",// j13 rtsp
    )
}

fun Context.uri2RealPath(uri: Uri): String {
    val result: String
    var cursor: Cursor? = null
    try {
        cursor = contentResolver.query(uri, null, null, null, null)
    } catch (e: Throwable) {
        e.printStackTrace()
    }
    if (cursor == null) {
        result = uri.path ?: ""
    } else {
        cursor.moveToFirst()
        val idx = cursor.getColumnIndex(MediaStore.Video.VideoColumns.DATA)
        result = cursor.getString(idx)
        cursor.close()
    }
    return result
}