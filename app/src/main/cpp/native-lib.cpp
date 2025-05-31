#include <jni.h>
#include <string>
#include "AndFFmpeg.h"
#include "AndCallJava.h"

_JavaVM *javaVM = NULL;
AndFFmpeg *ffmpeg = NULL;
AndCallJava *callJava = NULL;
AndPlayStatus *playStatus = NULL;

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
    if(ffmpeg != NULL)
    {
        ffmpeg->start();
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_andplayer_service_AndPlayer_n_1stop(JNIEnv *env, jobject thiz) {
    if(ffmpeg != NULL) {
        if (callJava != NULL) {
            delete callJava;
            callJava = NULL;
        }
        if (playStatus != NULL) {
            delete playStatus;
            playStatus = NULL;
        }

        ffmpeg->release();
        delete ffmpeg;
        ffmpeg = nullptr;
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_andplayer_service_AndPlayer_n_1pause(JNIEnv *env, jobject thiz) {
    if(ffmpeg != NULL)
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
    if (ffmpeg != NULL) {
        ffmpeg->seek(secds);
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_andplayer_service_AndPlayer_n_1resume(JNIEnv *env, jobject thiz) {
    if(ffmpeg != NULL)
    {
        ffmpeg->resume();
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_andplayer_service_AndPlayer_n_1mute(JNIEnv *env, jobject thiz, jint mute) {
    if(ffmpeg != NULL)
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
    if (ffmpeg != NULL)
    {
        ffmpeg->setSpeed(speed);
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_andplayer_service_AndPlayer_n_1setTone(JNIEnv *env, jobject thiz, jfloat tone) {
    if (ffmpeg != NULL)
    {
        ffmpeg->setTone(tone);
    }
}