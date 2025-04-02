//
// Created by BaiYang on 2023-04-12.
//

#ifndef MUSICPLAYER_ANDROIDLOG_H
#define MUSICPLAYER_ANDROIDLOG_H

#include <android/log.h>
#include <android/native_window_jni.h>

//#define LOGD(...) __android_log_print(ANDROID_LOG_INFO,"baiyang",__VA_ARGS__)

#define LOG_DEBUG true

#define LOGD(FORMAT, ...) __android_log_print(ANDROID_LOG_DEBUG,"native",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"native",FORMAT,##__VA_ARGS__);


#endif //MUSICPLAYER_ANDROIDLOG_H
