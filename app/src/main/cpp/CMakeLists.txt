# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html


cmake_minimum_required(VERSION 3.22.1)

project("andplayer")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
        andplayer
        SHARED
        native-lib.cpp)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

# ffmpeg头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# 添加系统环境变量  一个系统查找目录
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/armeabi-v7a")

target_link_libraries( # Specifies the target library.
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