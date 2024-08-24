# 设置cmake最低版本
cmake_minimum_required(VERSION 3.18.1)
# 设置GCC编译器的编译选项
if (CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-declarations -Wmissing-prototypes")
endif (CMAKE_COMPILER_IS_GNUCC)
# 设置clang编译器的编译选项
if (CMAKE_COMPILER_IS_CLANG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-declarations -Wmissing-prototypes -Wdocumentation -Wunreachable-code")
endif (CMAKE_COMPILER_IS_CLANG)

# 指定so库文件路径
set(JNI_LIBS_DIR ${CMAKE_SOURCE_DIR}/../jniLibs/${ANDROID_ABI})
# 设置头文件目录
include_directories(common include glm)

#av工具类库
add_library(avutil SHARED IMPORTED)
set_target_properties(avutil PROPERTIES IMPORTED_LOCATION ${JNI_LIBS_DIR}/libavutil.so)
#音频采样数据格式转换
add_library(swresample SHARED IMPORTED)
set_target_properties(swresample PROPERTIES IMPORTED_LOCATION ${JNI_LIBS_DIR}/libswresample.so)
#视频像素格式转换
add_library(swscale SHARED IMPORTED)
set_target_properties(swscale PROPERTIES IMPORTED_LOCATION ${JNI_LIBS_DIR}/libswscale.so)
#编解码（重要）
add_library(avcodec SHARED IMPORTED)
set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION ${JNI_LIBS_DIR}/libavcodec.so)
#封装格式处理libavdevice.so
add_library(avformat SHARED IMPORTED)
set_target_properties(avformat PROPERTIES IMPORTED_LOCATION ${JNI_LIBS_DIR}/libavformat.so)
#滤镜特效处理
add_library(avfilter SHARED IMPORTED)
set_target_properties(avfilter PROPERTIES IMPORTED_LOCATION ${JNI_LIBS_DIR}/libavfilter.so)
#各种设备的输入输出
add_library(avdevice SHARED IMPORTED)
set_target_properties(avdevice PROPERTIES IMPORTED_LOCATION ${JNI_LIBS_DIR}/libavdevice.so)

# metadata子库
#add_subdirectory(${CMAKE_SOURCE_DIR}/metadata)

#player子库
add_subdirectory(${CMAKE_SOURCE_DIR}/player)