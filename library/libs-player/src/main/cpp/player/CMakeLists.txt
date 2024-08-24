# 根据API版本判断使用哪个版本的OpenGLES
if (${ANDROID_PLATFORM_LEVEL} LESS 12) #应该是Android8以下不支持2.0的
    message(FATAL_ERROR "OpenGL 2 is not supported before API level 11 (currently using ${ANDROID_PLATFORM_LEVEL}).")
    return()
elseif (${ANDROID_PLATFORM_LEVEL} LESS 18) #Android版本低于18将使用OpenGL 2.0
    add_definitions("-DDYNAMIC_ES3")
    set(GLES-lib GLESv2)
else () #Android 18开始支持OpenGL ES3.0
    set(GLES-lib GLESv3)
endif (${ANDROID_PLATFORM_LEVEL} LESS 12)

# 添加 soundtouch 动态库
set(soundtouch_dir ../soundtouch)
add_subdirectory(${soundtouch_dir} soundtouch)

include_directories(
        source
        source/common
        source/convertor
        source/decoder/header
        source/device/header
        source/device/android/header
        source/player/header
        source/queue/header
        source/render/header
        source/render/common/header
        source/render/filter/header
        source/render/filter/input/header
        source/render/filter/adjust/header
        source/render/filter/beauty/header
        source/render/filter/effect/header
        source/render/filter/striker/header
        source/sync/header
        android/header
        ${soundtouch_dir}
)

file(GLOB player_source
        source/common/*.cpp
        source/convertor/*.cpp
        source/decoder/*.cpp
        source/device/*.cpp
        source/device/android/*.cpp
        source/queue/*.cpp
        source/recorder/*.cpp
        source/render/*.cpp
        source/render/common/*.cpp
        source/render/filter/*.cpp
        source/render/filter/input/*.cpp
        source/render/filter/adjust/*.cpp
        source/render/filter/beauty/*.cpp
        source/render/filter/effect/*.cpp
        source/render/filter/striker/*.cpp
        source/sync/*.cpp
        source/player/*.cpp
        android/*.cpp
)

add_library(player SHARED ${player_source})
target_link_libraries(
        player
        soundtouch
        avutil swresample swscale avcodec avformat avfilter avdevice
        -landroid -llog -lOpenSLES -lEGL
        ${GLES-lib}
)