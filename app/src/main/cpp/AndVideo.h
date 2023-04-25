//
// Created by BaiYang on 2023-04-20.
//

#ifndef MUSICPLAYER_ANDVIDEO_H
#define MUSICPLAYER_ANDVIDEO_H

#include "AndQueue.h"
#include "AndCallJava.h"
#include "AndPlayStatus.h"
#include "AndAudio.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
};

class AndVideo {
public:
    pthread_t thread_play;
    pthread_mutex_t codecMutex;

    AndAudio *vAudio = NULL;  // 视频里必须要有audio，要做到向audio同步
    AndQueue *queue = NULL;
    AndPlayStatus *playStatus = NULL;
    AndCallJava *callJava = NULL;

    int streamIndex = -1;
    AVCodecContext *codecCtx = NULL;
    AVCodecParameters *codecpar = NULL;

    double clock = 0;
    // 实时计算出来 视频延迟时间
    double delayTime = 0;
    // 默认延迟时间  若帧率是25，则为40ms; 30，则为33ms
    double defaultDelayTime = 0.04;
    AVRational time_base;
    // 同步阈值  音视频pts的差值与该阈值进行对比，再去决定是否同步
    double SyncThreshold = 0.003;


public:
    AndVideo(AndPlayStatus *playStatus, AndCallJava *callJava);
    ~AndVideo();

    void play();

    void pause();

    void resume();

    void release();


    double getFrameDiffTime(AVFrame *avFrame);

    double getDelayTime(double diff);
};

#endif //MUSICPLAYER_ANDVIDEO_H
