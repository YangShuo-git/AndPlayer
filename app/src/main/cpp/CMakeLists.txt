cmake_minimum_required(VERSION 3.22.1)

message(------CMake begin-------)
message(${CMAKE_CURRENT_SOURCE_DIR})

project("andplayer")

# 引入soundtouch的头文件与库
include_directories(soundtouch/include)
include_directories(soundtouch/SoundTouch)

# ffmpeg头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
# 设置ffmpeg库的查找目录 (下面就可以直接写库的名字)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_CURRENT_SOURCE_DIR}/jniLibs/${ANDROID_ABI}")

aux_source_directory(. SOURCE)
aux_source_directory(./soundtouch/SoundTouch SOURCE_ST)

add_library(
        andplayer
        SHARED

        ${SOURCE_ST}
        ${SOURCE})

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

target_link_libraries(
        andplayer

        avcodec
        avformat
        avutil
        avfilter
        swresample
        swscale

        android
        OpenSLES
        # Links the target library to the log library
        # included in the NDK.
        ${log-lib})
message(------CMake end-------)