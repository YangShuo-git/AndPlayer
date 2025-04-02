//
// Created by BaiYang on 2023-04-12.
//

#include "AndCallJava.h"

AndCallJava::AndCallJava(_JavaVM *javaVM, JNIEnv *env, jobject obj) {
    this->javaVM = javaVM;
    this->jniEnv = env;
    this->jobj = env->NewGlobalRef(obj);

    jclass jlz = jniEnv->GetObjectClass(jobj);
    jmid_prepared = env->GetMethodID(jlz, "onCallPrepared", "()V");
    jmid_timeinfo = env->GetMethodID(jlz, "onCallTimeInfo", "(II)V");
    jmid_load = env->GetMethodID(jlz, "onCallLoad", "(Z)V");
    jmid_renderyuv = env->GetMethodID(jlz, "onCallRenderYUV", "(II[B[B[B)V");
}

// type用来区分是主线程，还是子线程
void AndCallJava::onCallPrepared(int type) {
    if(type == MAIN_THREAD) {
        // Native层调用java层的方法(jobject、MethodID、参数)
        // 主线程的jniEnv， 由 JVM 自动管理，可直接使用
        jniEnv->CallVoidMethod(jobj, jmid_prepared);
    } else if(type == CHILD_THREAD) {
        JNIEnv *childJniEnv;
        // 子线程的JNIEnv，必须通过 AttachCurrentThread 获取
        if (javaVM->AttachCurrentThread(&childJniEnv, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv worng");
            }
            return;
        }
        childJniEnv->CallVoidMethod(jobj, jmid_prepared);
        // 必须分离线程，以避免资源泄漏
        javaVM->DetachCurrentThread();
    }
}

void AndCallJava::onCallTimeInfo(int type, int curr, int total) {
    if (type == MAIN_THREAD) {
        jniEnv->CallVoidMethod(jobj, jmid_timeinfo, curr, total);
    } else if (type == CHILD_THREAD) {
        JNIEnv *childJniEnv;
        if (javaVM->AttachCurrentThread(&childJniEnv, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("Fail to call onCallTimeInfo!");
            }
            return;
        }
        childJniEnv->CallVoidMethod(jobj, jmid_timeinfo, curr, total);
        javaVM->DetachCurrentThread();
    }
}

void AndCallJava::onCallLoad(int type, bool load) {

    if(type == MAIN_THREAD)
    {
        jniEnv->CallVoidMethod(jobj, jmid_load, load);
    }
    else if(type == CHILD_THREAD) {
        JNIEnv *childJniEnv;
        if (javaVM->AttachCurrentThread(&childJniEnv, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("Fail to call onCallLoad!");
            }
            return;
        }
        childJniEnv->CallVoidMethod(jobj, jmid_load, load);
        javaVM->DetachCurrentThread();
    }
}

void AndCallJava::onCallRenderYUV(int width, int height, uint8_t *fy, uint8_t *fu, uint8_t *fv) {

    JNIEnv *childJniEnv;
    if (javaVM->AttachCurrentThread(&childJniEnv, 0) != JNI_OK) {
        if (LOG_DEBUG) {
            LOGE("Fail to call onCallRenderYUV!");
        }
        return;
    }

    // yuv420的格式，计算y、u、v的大小
    jbyteArray y = childJniEnv->NewByteArray(width * height);
    childJniEnv->SetByteArrayRegion(y, 0, width * height, reinterpret_cast<const jbyte *>(fy));

    jbyteArray u = childJniEnv->NewByteArray(width * height / 4);
    childJniEnv->SetByteArrayRegion(u, 0, width * height / 4, reinterpret_cast<const jbyte *>(fu));

    jbyteArray v = childJniEnv->NewByteArray(width * height / 4);
    childJniEnv->SetByteArrayRegion(v, 0, width * height / 4, reinterpret_cast<const jbyte *>(fv));

    childJniEnv->CallVoidMethod(jobj, jmid_renderyuv, width, height, y, u, v);

    childJniEnv->DeleteLocalRef(y);
    childJniEnv->DeleteLocalRef(u);
    childJniEnv->DeleteLocalRef(v);
    javaVM->DetachCurrentThread();
}

