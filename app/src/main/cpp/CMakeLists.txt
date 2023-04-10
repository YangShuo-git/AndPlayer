cmake_minimum_required(VERSION 3.22.1)

project("andplayer")

add_library(
        andplayer
        SHARED
        native-lib.cpp)

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

# ffmpeg头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# 添加系统环境变量  设置ffmpeg库的查找目录
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_CURRENT_SOURCE_DIR}/jniLibs/${ANDROID_ABI}")

target_link_libraries(
        andplayer

        avcodec
        avformat
        avutil
        avfilter
        swresample
        swscale

        android

        # Links the target library to the log library
        # included in the NDK.
        ${log-lib})