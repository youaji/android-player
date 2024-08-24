# 设置GCC编译器的编译选项
if (CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-declarations -Wmissing-prototypes")
endif (CMAKE_COMPILER_IS_GNUCC)

# 设置clang编译器的编译选项
if (CMAKE_COMPILER_IS_CLANG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-declarations -Wmissing-prototypes -Wdocumentation -Wunreachable-code")
endif (CMAKE_COMPILER_IS_CLANG)

# 添加源文件
add_library(soundtouch SHARED
        # library
        SoundTouch/AAFilter.cpp
        SoundTouch/BPMDetect.cpp
        SoundTouch/cpu_detect_x86.cpp
        SoundTouch/FIFOSampleBuffer.cpp
        SoundTouch/FIRFilter.cpp
        SoundTouch/InterpolateCubic.cpp
        SoundTouch/InterpolateLinear.cpp
        SoundTouch/InterpolateShannon.cpp
        SoundTouch/mmx_optimized.cpp
        SoundTouch/PeakFinder.cpp
        SoundTouch/RateTransposer.cpp
        SoundTouch/SoundTouch.cpp
        SoundTouch/sse_optimized.cpp
        SoundTouch/TDStretch.cpp
        # wrapper
        SoundTouchWrapper.cpp
)

target_include_directories(soundtouch PRIVATE include /SoundTouch)
target_link_libraries(soundtouch android log)

























