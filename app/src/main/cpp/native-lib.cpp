#include <jni.h>
#include <string>
#include "AndFFmpeg.h"
#include "AndCallJava.h"

_JavaVM *javaVM = nullptr;
AndFFmpeg *ffmpeg = nullptr;
AndCallJava *callJava = nullptr;
AndPlayStatus *playStatus = nullptr;

extern "C"
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    jint result = -1;
    javaVM = vm;
    JNIEnv *env;
    if(vm->GetEnv((void **)&env, JNI_VERSION_1_4) != JNI_OK)
    {
        return result;
    }
    return JNI_VERSION_1_4;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_andplayer_service_AndPlayer_n_1prepared(JNIEnv *env, jobject thiz,
                                                         jstring _source) {
    const char *source = env->GetStringUTFChars(_source, 0);

    if (ffmpeg == nullptr) {
        if (callJava == nullptr) {
            callJava = new AndCallJava(javaVM, env, thiz);
        }
        playStatus = new AndPlayStatus();
        ffmpeg = new AndFFmpeg(playStatus, callJava, source);

        ffmpeg->prepared();
    }

    env->ReleaseStringUTFChars(_source, source);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_andplayer_service_AndPlayer_n_1start(JNIEnv *env, jobject thiz) {
    if (ffmpeg != nullptr)
    {
        ffmpeg->start();
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_andplayer_service_AndPlayer_n_1stop(JNIEnv *env, jobject thiz) {
    if (ffmpeg != nullptr) {
        if (callJava != nullptr) {
            delete callJava;
            callJava = nullptr;
        }
        if (playStatus != nullptr) {
            delete playStatus;
            playStatus = nullptr;
        }

        ffmpeg->release();
        delete ffmpeg;
        ffmpeg = nullptr;
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_andplayer_service_AndPlayer_n_1pause(JNIEnv *env, jobject thiz) {
    if (ffmpeg != nullptr)
    {
        if (!playStatus->isPaused) {
            ffmpeg->pause();
        }
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_andplayer_service_AndPlayer_n_1seek(JNIEnv *env, jobject thiz, jint secds) {
    LOGE("seek to %d  ", secds);
    if (ffmpeg != nullptr) {
        ffmpeg->seek(secds);
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_andplayer_service_AndPlayer_n_1resume(JNIEnv *env, jobject thiz) {
    if (ffmpeg != nullptr)
    {
        ffmpeg->resume();
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_andplayer_service_AndPlayer_n_1mute(JNIEnv *env, jobject thiz, jint mute) {
    if (ffmpeg != nullptr)
    {
        ffmpeg->setMute(mute);
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_andplayer_service_AndPlayer_n_1volume(JNIEnv *env, jobject thiz, jint percent) {
    // TODO: implement n_volume()
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_andplayer_service_AndPlayer_n_1speed(JNIEnv *env, jobject thiz, jfloat speed) {
    if (ffmpeg != nullptr)
    {
        ffmpeg->setSpeed(speed);
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_andplayer_service_AndPlayer_n_1setTone(JNIEnv *env, jobject thiz, jfloat tone) {
    if (ffmpeg != nullptr)
    {
        ffmpeg->setTone(tone);
    }
}