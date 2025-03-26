cmake_minimum_required(VERSION 3.22.1)

message(------CMake begin-------)
message(${CMAKE_CURRENT_SOURCE_DIR})

project("andplayer")

# ffmpeg是直接引入so库，需要头文件
# opensl是引入源文件，在cmake中进行编译，和native-lib.cpp等文件编入了andplayer

# 引入ffmpeg头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
# 设置ffmpeg库的查找目录 (作用就是下面就可以直接写ffmpeg库的名字)
#   CMAKE_CXX_FLAGS用于存储 C++ 编译器的标志
#   作用是将 JNI 库文件所在的路径添加到 C++ 编译器的标志中，以便编译器在链接阶段能够正确地查找和链接 JNI 库
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_CURRENT_SOURCE_DIR}/jniLibs/${ANDROID_ABI}")

# 引入soundtouch的头文件与库
include_directories(soundtouch/include)
include_directories(soundtouch/SoundTouch)

aux_source_directory(. SOURCE)
aux_source_directory(./soundtouch/SoundTouch SOURCE_ST)

add_library(
        andplayer
        SHARED

        ${SOURCE_ST}
        ${SOURCE})

# 作用是在指定的搜索路径中查找名为 log 的库文件，并将找到的库文件的路径存储在 log-lib 变量中
# 一旦找到 log 库，就可以在下面直接使用 log-lib 变量来引用该库文件。
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