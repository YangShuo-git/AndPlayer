#include <jni.h>
#include <string>

extern "C" {
#include "include/libavcodec/avcodec.h"
#include "include/libavutil/avutil.h"
#include "include/libavfilter/avfilter.h"
#include "include/libavformat/avformat.h"
#include "include/libswresample/swresample.h"
#include "include/libswscale/swscale.h"
}

#include <android/log.h>
#include <android/native_window_jni.h>

#define LOGD(...) __android_log_print(ANDROID_LOG_INFO,"baiyang",__VA_ARGS__)

static AVFormatContext *avFormatContext;
static AVCodecContext *avCodecContext;
AVCodec *vCodec;
ANativeWindow* nativeWindow;
ANativeWindow_Buffer windowBuffer;

static AVPacket *avPacket;
static AVFrame *avFrame, *rgbFrame;
struct SwsContext *swsContext;
uint8_t *outbuffer;

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_andplayer_MainActivity_play(JNIEnv *env, jobject thiz, jstring url_,
                                              jobject surface) {
    const char *url = env->GetStringUTFChars(url_, 0);
    avFormatContext = avformat_alloc_context();
    //打开文件
    if(avformat_open_input(&avFormatContext, url, NULL, NULL) != 0){
        LOGD("Couldn't open input stream.\n");
        return -1;
    }
    LOGD("打开视频成功.\n");

    avformat_free_context(avFormatContext);
    env->ReleaseStringUTFChars(url_, url);
    return 0;
}